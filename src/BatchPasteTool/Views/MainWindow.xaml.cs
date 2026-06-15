using System.Runtime.InteropServices;
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
    //  WNDPROC HOOK
    //  WindowChrome handles resize hit-testing (WM_NCHITTEST).
    //  This hook handles:
    //    1. WM_NCCALCSIZE — force client area = window area, fixing
    //       top/left resize flicker caused by async position+size update
    //    2. WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE — suppress ScrollBar
    //       layout updates during resize
    // ================================================================

    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);
        var source = HwndSource.FromHwnd(new WindowInteropHelper(this).Handle);
        source?.AddHook(WndProc);
    }

    private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
    {
        const int WM_NCCALCSIZE = 0x0083;
        const int WM_ENTERSIZEMOVE = 0x0231;
        const int WM_EXITSIZEMOVE = 0x0232;

        // ── WM_NCCALCSIZE ──────────────────────────────────────────
        // Remove the non-client area entirely so the DWM does not
        // draw any border and WPF fills the full window rect.
        // wParam==TRUE: lParam → NCCALCSIZE_PARAMS
        //   offset  0: rgrc[0]  new client rect (output)
        //   offset 32: rgrc[2]  new window rect
        // Copy window rect → client rect (no non-client area).
        if (msg == WM_NCCALCSIZE)
        {
            if (wParam != IntPtr.Zero)
            {
                Marshal.WriteInt32(lParam,  0, Marshal.ReadInt32(lParam, 32)); // left
                Marshal.WriteInt32(lParam,  4, Marshal.ReadInt32(lParam, 36)); // top
                Marshal.WriteInt32(lParam,  8, Marshal.ReadInt32(lParam, 40)); // right
                Marshal.WriteInt32(lParam, 12, Marshal.ReadInt32(lParam, 44)); // bottom
                handled = true;
                return (IntPtr)0x0400; // WVR_VALIDRECTS
            }
            else
            {
                handled = true;
                return IntPtr.Zero;
            }
        }

        // ── WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE ─────────────────────
        if (msg == WM_ENTERSIZEMOVE)
        {
            _isResizing = true;
        }
        else if (msg == WM_EXITSIZEMOVE)
        {
            _isResizing = false;
            Dispatcher.BeginInvoke(new Action(UpdateScrollbarRange),
                System.Windows.Threading.DispatcherPriority.Loaded);
        }

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
