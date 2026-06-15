using System.Windows.Threading;
using BatchPasteTool.Helpers;

namespace BatchPasteTool.Services;

public interface IForegroundWindowService
{
    IntPtr LastKnownTargetWindow { get; }
    void Start(IntPtr myWindowHandle);
    void Stop();
}

public class ForegroundWindowService : IForegroundWindowService
{
    private DispatcherTimer? _timer;
    private IntPtr _myWindowHandle;
    public IntPtr LastKnownTargetWindow { get; private set; } = IntPtr.Zero;

    public void Start(IntPtr myWindowHandle)
    {
        _myWindowHandle = myWindowHandle;
        _timer = new DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(Constants.ForegroundCheckMs)
        };
        _timer.Tick += OnTick;
        _timer.Start();
    }

    public void Stop()
    {
        _timer?.Stop();
        _timer = null;
    }

    private void OnTick(object? sender, EventArgs e)
    {
        IntPtr fg = NativeMethods.GetForegroundWindow();
        if (fg != IntPtr.Zero && fg != _myWindowHandle)
            LastKnownTargetWindow = fg;
    }
}
