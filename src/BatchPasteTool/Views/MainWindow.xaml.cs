using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Interop;
using BatchPasteTool.Helpers;
using BatchPasteTool.ViewModels;

namespace BatchPasteTool.Views;

public partial class MainWindow : Window
{
    private MainViewModel VM => (MainViewModel)DataContext;
    private const int ResizeBorder = 6;
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
        // Defer scrollbar update until well after layout completes.
        // VirtualizingStackPanel doesn't report correct ExtentHeight
        // at Loaded priority; Background gives it time to measure.
        Dispatcher.BeginInvoke(new Action(UpdateScrollbarRange),
            System.Windows.Threading.DispatcherPriority.Background);
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
    //    1. WM_NCHITTEST — resize borders (8 edges/corners)
    //    2. WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE — suppress ScrollBar
    //       layout updates during resize
    // ================================================================

    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);
        IntPtr hwnd = new WindowInteropHelper(this).Handle;

        var source = HwndSource.FromHwnd(hwnd);
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
            Dispatcher.BeginInvoke(new Action(UpdateScrollbarRange),
                System.Windows.Threading.DispatcherPriority.Loaded);
        }

        if (msg == NativeMethods.WM_NCHITTEST)
        {
            int screenX = (int)(lParam.ToInt64() & 0xFFFF);
            int screenY = (int)(lParam.ToInt64() >> 16);
            Point pt = PointFromScreen(new Point(screenX, screenY));

            // Resize borders (8 edges / 4 corners)
            if (pt.X <= ResizeBorder && pt.Y <= ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTTOPLEFT); }
            if (pt.X >= ActualWidth - ResizeBorder && pt.Y <= ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTTOPRIGHT); }
            if (pt.X <= ResizeBorder && pt.Y >= ActualHeight - ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTBOTTOMLEFT); }
            if (pt.X >= ActualWidth - ResizeBorder && pt.Y >= ActualHeight - ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTBOTTOMRIGHT); }
            if (pt.X <= ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTLEFT); }
            if (pt.X >= ActualWidth - ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTRIGHT); }
            if (pt.Y <= ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTTOP); }
            if (pt.Y >= ActualHeight - ResizeBorder)
            { handled = true; return new IntPtr(NativeMethods.HTBOTTOM); }
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

    /// <summary>
    /// Some systems / scenarios the VirtualizingStackPanel IScrollInfo
    /// doesn't forward the mouse wheel to the ScrollViewer.  Handle it
    /// ourselves so the wheel always scrolls the list.
    /// </summary>
    private void ContentScroller_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        double delta = e.Delta > 0 ? -42 : 42; // one item row per tick
        double newOffset = ContentScroller.VerticalOffset + delta;
        double max = ContentScroller.ScrollableHeight;
        if (max <= 0) max = VertScrollbar.Maximum;
        newOffset = Math.Max(0, Math.Min(newOffset, max));
        ContentScroller.ScrollToVerticalOffset(newOffset);
        UpdateScrollbarRange();
        e.Handled = true;
    }

    public void UpdateScrollbarRange()
    {
        // VirtualizingStackPanel may report ExtentHeight == 0 or == ViewportHeight
        // before any item is realized.  Fall back to a computed extent from item count.
        double extent = ContentScroller.ExtentHeight;
        double viewport = ContentScroller.ViewportHeight;

        if (extent <= 0 || Math.Abs(extent - viewport) < 0.5)
        {
            // Each item row: Grid Height=40 + Margin.Bottom=2 = 42 px
            extent = VM.Items.Count * 42;
        }

        // ViewportHeight may also be 0 before the first layout pass
        // (e.g. when called from LoadState via SyncScroll).
        if (viewport <= 0)
        {
            viewport = ActualHeight - Constants.TitleH - Constants.BottomH - 2; // 2 = border
            if (viewport <= 0) viewport = 400; // sensible default
        }

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
