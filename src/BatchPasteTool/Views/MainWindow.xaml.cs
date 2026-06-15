using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using BatchPasteTool.Helpers;
using BatchPasteTool.ViewModels;

namespace BatchPasteTool.Views;

public partial class MainWindow : Window
{
    private MainViewModel VM => (MainViewModel)DataContext;
    private const int ResizeBorder = 6;

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

    // ================================================================
    //  WNDPROC HOOK (Hit-testing for resize & drag)
    // ================================================================

    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);
        var source = HwndSource.FromHwnd(new WindowInteropHelper(this).Handle);
        source?.AddHook(WndProc);
    }

    private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
    {
        if (msg == NativeMethods.WM_NCHITTEST)
        {
            int screenX = (int)(lParam.ToInt64() & 0xFFFF);
            int screenY = (int)(lParam.ToInt64() >> 16);
            Point pt = PointFromScreen(new Point(screenX, screenY));

            // Only handle resize borders — title bar drag uses MouseLeftButtonDown
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
