using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Interop;
using BatchPasteTool.ViewModels;

namespace BatchPasteTool.Views;

public partial class MainWindow : Window
{
    private MainViewModel VM => (MainViewModel)DataContext;
    private bool _isResizing;

    public MainWindow()
    {
        InitializeComponent();
    }

    // ================================================================
    //  WINDOW LIFECYCLE
    // ================================================================

    private void Window_Loaded(object sender, RoutedEventArgs e)
    {
        VM.Initialize(this);
        UpdateScrollbarRange();
    }

    private void Window_Closing(object? sender, System.ComponentModel.CancelEventArgs e)
    {
        VM.Shutdown();
    }

    private void Window_Deactivated(object? sender, EventArgs e)
    {
        VM.OnWindowDeactivated();
    }

    // ================================================================
    //  WNDPROC HOOK — detect resize start/end to suppress layout
    //  thrashing.  WindowChrome handles resize hit-testing natively;
    //  this hook only watches WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE.
    // ================================================================

    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);
        var source = HwndSource.FromHwnd(new WindowInteropHelper(this).Handle);
        source?.AddHook(WndProc);
    }

    private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
    {
        const int WM_ENTERSIZEMOVE = 0x0231;
        const int WM_EXITSIZEMOVE = 0x0232;

        if (msg == WM_ENTERSIZEMOVE)
        {
            _isResizing = true;
        }
        else if (msg == WM_EXITSIZEMOVE)
        {
            _isResizing = false;
            // One final sync after the resize layout settles.
            Dispatcher.BeginInvoke(new Action(UpdateScrollbarRange),
                System.Windows.Threading.DispatcherPriority.Loaded);
        }

        // Never mark handled — WindowChrome needs these messages too.
        return IntPtr.Zero;
    }

    // ================================================================
    //  TITLE BAR DRAG
    // ================================================================

    private void TitleBar_MouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left && e.ClickCount == 1)
            DragMove();
    }

    // ================================================================
    //  SCROLLBAR
    // ================================================================

    private void VertScrollbar_Scroll(object sender, ScrollEventArgs e)
    {
        ContentScroller.ScrollToVerticalOffset(e.NewValue);
    }

    private void ContentScroller_ScrollChanged(object sender, ScrollChangedEventArgs e)
    {
        // During window resize the viewport changes on every pixel.
        // Updating the ScrollBar properties triggers extra layout passes
        // that cascade to the bottom bar, causing visible flicker.
        // WM_EXITSIZEMOVE does a final sync when the drag ends.
        if (!_isResizing)
            UpdateScrollbarRange();
    }

    public void UpdateScrollbarRange()
    {
        double extent = ContentScroller.ExtentHeight;
        double viewport = ContentScroller.ViewportHeight;
        VertScrollbar.Minimum = 0;
        VertScrollbar.Maximum = Math.Max(0, extent - viewport);
        VertScrollbar.ViewportSize = viewport;
        VertScrollbar.Value = ContentScroller.VerticalOffset;
    }

    // ================================================================
    //  KEYBOARD SHORTCUTS
    // ================================================================

    private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
    {
        if (Keyboard.Modifiers == ModifierKeys.Control && e.Key == Key.Z)
        {
            VM.Undo();
            e.Handled = true;
        }
        else if (Keyboard.Modifiers == ModifierKeys.Control && e.Key == Key.N)
        {
            VM.AddItem();
            e.Handled = true;
        }
        else if (Keyboard.Modifiers == (ModifierKeys.Control | ModifierKeys.Shift) && e.Key == Key.S)
        {
            VM.SendAllItems();
            e.Handled = true;
        }
    }

    // ================================================================
    //  ITEM TEXTBOX FOCUS (Undo tracking)
    // ================================================================

    private void ItemTextBox_GotFocus(object sender, RoutedEventArgs e)
    {
        if (sender is TextBox tb && tb.DataContext is PasteItemViewModel item)
        {
            item.SaveFocusText();
        }
    }

    private void ItemTextBox_LostFocus(object sender, RoutedEventArgs e)
    {
        if (sender is TextBox tb && tb.DataContext is PasteItemViewModel item)
        {
            VM.OnItemFocusLost(item);
        }
    }
}
