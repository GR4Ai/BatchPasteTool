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
            // Get screen coordinates
            int screenX = (int)(lParam.ToInt64() & 0xFFFF);
            int screenY = (int)(lParam.ToInt64() >> 16);

            // Convert to client coordinates
            Point pt = PointFromScreen(new Point(screenX, screenY));

            bool inTitleBar = pt.Y >= 0 && pt.Y < Constants.TitleH;

            // Check if over interactive elements in title bar
            if (inTitleBar)
            {
                // Check each button (approximate rects)
                if (IsOverInteractiveTitleElement(pt))
                {
                    handled = true;
                    return new IntPtr(NativeMethods.HTCLIENT);
                }
                // Allow drag on rest of title bar
                handled = true;
                return new IntPtr(NativeMethods.HTCAPTION);
            }

            // Border resize zones
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

    /// <summary>
    /// Quick check if a client point is over a title bar interactive element.
    /// Elements are now spread across the title bar: pin on left, transparency in middle,
    /// slider + window buttons on right.
    /// </summary>
    private static bool IsOverInteractiveTitleElement(Point pt)
    {
        // Left side: pin button area (name text ~140px + pin 24px)
        if (pt.X >= 140 && pt.X <= 174 && pt.Y < Constants.TitleH) return true;
        // Middle: transparency icon (centered in the title bar, ~24px at center)
        double midCenter = 620 / 2.0;
        if (pt.X >= midCenter - 16 && pt.X <= midCenter + 16 && pt.Y < Constants.TitleH) return true;
        // Right side: slider (140px) + window buttons (3 × 46px)
        if (pt.X > (620 - 3 * Constants.WinBtnW - Constants.SliderW - 30)) return true;
        return false;
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
