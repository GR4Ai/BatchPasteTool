using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Threading;
using BatchPasteTool.Helpers;
using BatchPasteTool.Models;
using BatchPasteTool.Services;

namespace BatchPasteTool.ViewModels;

public class MainViewModel : INotifyPropertyChanged
{
    // ---- Services ----
    private readonly IClipboardService _clipboard;
    private readonly IInputSimulationService _inputSim;
    private readonly IConfigService _config;
    private readonly IForegroundWindowService _foreground;
    private readonly IUndoService _undo;

    // ---- Timers ----
    private readonly DispatcherTimer _autoSaveTimer;

    // ---- Window reference (set after creation) ----
    private Window? _window;

    // ---- Collections ----
    public ObservableCollection<PasteItemViewModel> Items { get; } = new();

    // ---- Bound Properties ----

    private bool _isPinned;
    public bool IsPinned
    {
        get => _isPinned;
        set { if (_isPinned != value) { _isPinned = value; OnPropertyChanged(); ApplyPin(); } }
    }

    private double _windowOpacity = 1.0;
    public double WindowOpacity
    {
        get => _windowOpacity;
        set { if (Math.Abs(_windowOpacity - value) > 0.001) { _windowOpacity = value; OnPropertyChanged(); } }
    }

    private int _transparencyValue = 255;
    public int TransparencyValue
    {
        get => _transparencyValue;
        set
        {
            if (_transparencyValue != value)
            {
                _transparencyValue = Math.Clamp(value, Constants.TranspMin, Constants.TranspMax);
                OnPropertyChanged();
                ApplyTransparency();
            }
        }
    }

    private double _sliderRatio;
    public double SliderRatio
    {
        get => _sliderRatio;
        set
        {
            if (Math.Abs(_sliderRatio - value) > 0.001)
            {
                _sliderRatio = Math.Clamp(value, 0.0, 1.0);
                // Left(0)=Opaque(255), Right(1)=80%Transparent(51)
                TransparencyValue = Constants.TranspMax - (int)(_sliderRatio * (Constants.TranspMax - Constants.TranspMin));
                OnPropertyChanged();
            }
        }
    }

    private bool _canUndo;
    public bool CanUndo
    {
        get => _canUndo;
        private set { if (_canUndo != value) { _canUndo = value; OnPropertyChanged(); } }
    }

    private bool _canDelete;
    public bool CanDelete
    {
        get => _canDelete;
        private set { if (_canDelete != value) { _canDelete = value; OnPropertyChanged(); } }
    }

    private string _windowTitle = Constants.AppName;
    public string WindowTitle
    {
        get => _windowTitle;
        set { if (_windowTitle != value) { _windowTitle = value; OnPropertyChanged(); } }
    }

    // ---- Scroll position ----
    private double _scrollOffset;
    public double ScrollOffset
    {
        get => _scrollOffset;
        set { if (Math.Abs(_scrollOffset - value) > 0.5) { _scrollOffset = value; OnPropertyChanged(); } }
    }

    private double _scrollMax;
    public double ScrollMax
    {
        get => _scrollMax;
        set { if (Math.Abs(_scrollMax - value) > 0.5) { _scrollMax = value; OnPropertyChanged(); } }
    }

    // ---- Commands ----
    public RelayCommand AddItemCommand { get; }
    public RelayCommand DeleteItemCommand { get; }
    public RelayCommand UndoCommand { get; }
    public RelayCommand ClearAllItemsCommand { get; }
    public RelayCommand ClearAllTextsCommand { get; }
    public RelayCommand TogglePinCommand { get; }
    public RelayCommand ToggleTransparencyCommand { get; }
    public RelayCommand MinimizeCommand { get; }
    public RelayCommand MaximizeRestoreCommand { get; }
    public RelayCommand CloseCommand { get; }
    public RelayCommand<PasteItemViewModel> SendItemCommand { get; }
    public RelayCommand<PasteItemViewModel> ClearItemCommand { get; }

    public MainViewModel(
        IClipboardService clipboard,
        IInputSimulationService inputSim,
        IConfigService config,
        IForegroundWindowService foreground,
        IUndoService undo)
    {
        _clipboard = clipboard;
        _inputSim = inputSim;
        _config = config;
        _foreground = foreground;
        _undo = undo;

        // ---- Commands ----
        AddItemCommand = new RelayCommand(AddItem);
        DeleteItemCommand = new RelayCommand(DeleteItem, () => CanDelete);
        UndoCommand = new RelayCommand(Undo, () => CanUndo);
        ClearAllItemsCommand = new RelayCommand(ClearAllItems);
        ClearAllTextsCommand = new RelayCommand(ClearAllTexts);
        TogglePinCommand = new RelayCommand(TogglePin);
        ToggleTransparencyCommand = new RelayCommand(ToggleTransparency);
        MinimizeCommand = new RelayCommand(Minimize);
        MaximizeRestoreCommand = new RelayCommand(MaximizeRestore);
        CloseCommand = new RelayCommand(Close);
        SendItemCommand = new RelayCommand<PasteItemViewModel>(SendItem);
        ClearItemCommand = new RelayCommand<PasteItemViewModel>(ClearItem);

        // ---- Auto-save timer ----
        _autoSaveTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(Constants.AutoSaveMs)
        };
        _autoSaveTimer.Tick += (_, _) => SaveState();

        UpdateCanStates();
    }

    // ================================================================
    //  INITIALIZATION
    // ================================================================

    public void Initialize(Window window)
    {
        _window = window;

        // Load saved state
        LoadState();

        // Start foreground tracking
        var helper = new System.Windows.Interop.WindowInteropHelper(window);
        IntPtr hwnd = helper.EnsureHandle();
        _foreground.Start(hwnd);

        // Start auto-save
        _autoSaveTimer.Start();

        // Apply saved pin/transparency
        ApplyPin();
        ApplyTransparency();
    }

    public void Shutdown()
    {
        _autoSaveTimer.Stop();
        _foreground.Stop();
        SaveState();
    }

    // ================================================================
    //  ITEM MANAGEMENT
    // ================================================================

    public void AddItem()
    {
        var item = new PasteItemViewModel { Index = Items.Count };
        Items.Add(item);

        _undo.Push(new UndoEntry
        {
            Type = UndoType.ItemAdded,
            ItemIndex = item.Index
        });

        UpdateCanStates();
        SyncScroll();
    }

    public void DeleteItem()
    {
        if (Items.Count <= 1) return;
        int lastIdx = Items.Count - 1;

        _undo.Push(new UndoEntry
        {
            Type = UndoType.ItemDeleted,
            ItemIndex = lastIdx,
            OldItemTexts = new List<string> { Items[lastIdx].Text }
        });

        Items.RemoveAt(lastIdx);
        // Re-index
        for (int i = 0; i < Items.Count; i++) Items[i].Index = i;

        UpdateCanStates();
        SyncScroll();
    }

    public void ClearItem(PasteItemViewModel? item)
    {
        if (item == null) return;

        string oldText = item.Text;
        if (string.IsNullOrEmpty(oldText)) return;

        _undo.Push(new UndoEntry
        {
            Type = UndoType.TextChanged,
            ItemIndex = item.Index,
            OldText = oldText,
            NewText = ""
        });

        item.Text = "";
        UpdateCanStates();
    }

    public void ClearAllItems()
    {
        var allTexts = Items.Select(it => it.Text).ToList();

        _undo.Push(new UndoEntry
        {
            Type = UndoType.AllItemsCleared,
            OldItemTexts = allTexts
        });

        // Remove all but first, clear first
        while (Items.Count > 1)
            Items.RemoveAt(Items.Count - 1);
        Items[0].Text = "";
        Items[0].Index = 0;

        UpdateCanStates();
        SyncScroll();
    }

    public void ClearAllTexts()
    {
        var allTexts = Items.Select(it => it.Text).ToList();

        _undo.Push(new UndoEntry
        {
            Type = UndoType.AllTextsCleared,
            OldItemTexts = allTexts
        });

        foreach (var item in Items)
            item.Text = "";

        UpdateCanStates();
    }

    public void OnWindowDeactivated()
    {
        _foreground.OnWindowDeactivated();
    }

    // ================================================================
    //  SEND / PASTE
    // ================================================================

    public void SendItem(PasteItemViewModel? item)
    {
        if (item == null || string.IsNullOrEmpty(item.Text)) return;

        _clipboard.SetText(item.Text);

        var target = _foreground.LastKnownTargetWindow;
        if (target != IntPtr.Zero && NativeMethods.IsWindow(target))
        {
            // AttachThreadInput to make SetForegroundWindow succeed instantly
            IntPtr fg = NativeMethods.GetForegroundWindow();
            uint foreThread = NativeMethods.GetWindowThreadProcessId(fg, out _);
            uint curThread = NativeMethods.GetCurrentThreadId();

            bool attached = false;
            if (foreThread != 0 && foreThread != curThread)
                attached = NativeMethods.AttachThreadInput(curThread, foreThread, true);

            NativeMethods.SetForegroundWindow(target);

            if (attached)
                NativeMethods.AttachThreadInput(curThread, foreThread, false);

            // Small sleep to let target activate before sending keys
            Thread.Sleep(30);
        }

        _inputSim.SimulateCtrlV();
    }

    public void SendAllItems()
    {
        var target = _foreground.LastKnownTargetWindow;
        if (target == IntPtr.Zero || !NativeMethods.IsWindow(target)) return;

        IntPtr fg = NativeMethods.GetForegroundWindow();
        uint foreThread = NativeMethods.GetWindowThreadProcessId(fg, out _);
        uint curThread = NativeMethods.GetCurrentThreadId();

        bool attached = false;
        if (foreThread != 0 && foreThread != curThread)
            attached = NativeMethods.AttachThreadInput(curThread, foreThread, true);

        foreach (var item in Items)
        {
            if (!string.IsNullOrEmpty(item.Text))
            {
                _clipboard.SetText(item.Text);
                NativeMethods.SetForegroundWindow(target);
                Thread.Sleep(30);
                _inputSim.SimulateCtrlV();
                Thread.Sleep(60);
            }
        }

        if (attached)
            NativeMethods.AttachThreadInput(curThread, foreThread, false);
    }

    // ================================================================
    //  UNDO
    // ================================================================

    public void Undo()
    {
        var entry = _undo.Pop();
        if (entry == null) return;

        switch (entry.Type)
        {
            case UndoType.TextChanged:
                if (entry.ItemIndex >= 0 && entry.ItemIndex < Items.Count)
                    Items[entry.ItemIndex].Text = entry.OldText ?? "";
                break;

            case UndoType.ItemAdded:
                if (Items.Count > 1)
                    Items.RemoveAt(Items.Count - 1);
                for (int i = 0; i < Items.Count; i++) Items[i].Index = i;
                break;

            case UndoType.ItemDeleted:
                var newItem = new PasteItemViewModel { Index = Items.Count };
                if (entry.OldItemTexts?.Count > 0)
                    newItem.Text = entry.OldItemTexts[0];
                Items.Add(newItem);
                break;

            case UndoType.AllItemsCleared:
                Items.Clear();
                if (entry.OldItemTexts != null)
                {
                    for (int i = 0; i < entry.OldItemTexts.Count; i++)
                    {
                        var it = new PasteItemViewModel { Index = i };
                        it.Text = entry.OldItemTexts[i];
                        Items.Add(it);
                    }
                }
                break;

            case UndoType.AllTextsCleared:
                if (entry.OldItemTexts != null)
                {
                    for (int i = 0; i < Math.Min(entry.OldItemTexts.Count, Items.Count); i++)
                        Items[i].Text = entry.OldItemTexts[i];
                }
                break;
        }

        UpdateCanStates();
        SyncScroll();
    }

    /// <summary>
    /// Track text changes on focus lost — push undo if text changed.
    /// </summary>
    public void OnItemFocusLost(PasteItemViewModel item)
    {
        if (item.GetTextChange(out string oldText, out string newText))
        {
            _undo.Push(new UndoEntry
            {
                Type = UndoType.TextChanged,
                ItemIndex = item.Index,
                OldText = oldText,
                NewText = newText
            });
            UpdateCanStates();
        }
    }

    // ================================================================
    //  PIN / TRANSPARENCY
    // ================================================================

    private void TogglePin()
    {
        IsPinned = !IsPinned;
    }

    private void ApplyPin()
    {
        if (_window == null) return;

        var helper = new System.Windows.Interop.WindowInteropHelper(_window);
        IntPtr hwnd = helper.EnsureHandle();

        NativeMethods.SetWindowPos(hwnd,
            IsPinned ? NativeMethods.HWND_TOPMOST : NativeMethods.HWND_NOTOPMOST,
            0, 0, 0, 0,
            NativeMethods.SWP_NOMOVE | NativeMethods.SWP_NOSIZE | NativeMethods.SWP_NOACTIVATE);

        _window.Topmost = IsPinned;
    }

    private void ToggleTransparency()
    {
        if (TransparencyValue >= Constants.TranspMax)
            TransparencyValue = Constants.TranspToggle;  // Toggle to semi-transparent
        else
            TransparencyValue = Constants.TranspMax;     // Toggle to fully opaque

        SliderRatio = (double)(Constants.TranspMax - TransparencyValue) /
                      (Constants.TranspMax - Constants.TranspMin);
    }

    private void ApplyTransparency()
    {
        WindowOpacity = TransparencyValue / (double)Constants.TranspMax;
        if (_window != null)
            _window.Opacity = WindowOpacity;
    }

    private void Minimize()
    {
        if (_window != null)
            _window.WindowState = WindowState.Minimized;
    }

    private void MaximizeRestore()
    {
        if (_window != null)
            _window.WindowState = _window.WindowState == WindowState.Maximized
                ? WindowState.Normal
                : WindowState.Maximized;
    }

    private void Close()
    {
        Shutdown();
        Application.Current.Shutdown();
    }

    // ================================================================
    //  STATE PERSISTENCE
    // ================================================================

    private void SaveState()
    {
        if (_window == null) return;

        var settings = new AppSettings
        {
            Pinned = IsPinned,
            Transparency = TransparencyValue,
            WinX = (int)_window.Left,
            WinY = (int)_window.Top,
            WinW = (int)_window.ActualWidth,
            WinH = (int)_window.ActualHeight,
            ItemTexts = Items.Select(it => it.Text).ToList()
        };

        _config.Save(settings);
    }

    private void LoadState()
    {
        var settings = _config.Load();

        IsPinned = settings.Pinned;
        TransparencyValue = settings.Transparency;
        SliderRatio = (double)(Constants.TranspMax - TransparencyValue) /
                      (Constants.TranspMax - Constants.TranspMin);

        // Create items
        Items.Clear();
        for (int i = 0; i < settings.ItemTexts.Count; i++)
        {
            var item = new PasteItemViewModel { Index = i };
            item.Text = settings.ItemTexts[i];
            Items.Add(item);
        }

        // Ensure at least 1 item
        if (Items.Count == 0)
        {
            Items.Add(new PasteItemViewModel { Index = 0 });
        }

        // Apply window position/size
        if (_window != null)
        {
            if (settings.WinX != int.MinValue && settings.WinY != int.MinValue)
            {
                _window.Left = settings.WinX;
                _window.Top = settings.WinY;
            }
            _window.Width = settings.WinW;
            _window.Height = settings.WinH;
        }

        // Clear undo stack populated during load
        _undo.Clear();

        UpdateCanStates();
        SyncScroll();
    }

    // ================================================================
    //  HELPERS
    // ================================================================

    private void UpdateCanStates()
    {
        CanUndo = _undo.CanUndo;
        CanDelete = Items.Count > 1;
    }

    public void SyncScroll()
    {
        double totalH = Items.Count * (Constants.ItemH + Constants.ItemGap) + 8;
        ScrollMax = Math.Max(0, totalH);
    }

    // ================================================================
    //  INotifyPropertyChanged
    // ================================================================

    public event PropertyChangedEventHandler? PropertyChanged;

    protected void OnPropertyChanged([CallerMemberName] string? name = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}
