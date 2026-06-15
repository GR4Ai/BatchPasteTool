using System.Runtime.InteropServices;
using BatchPasteTool.Helpers;

namespace BatchPasteTool.Services;

public interface IInputSimulationService
{
    void SimulateCtrlV();
    void PostCtrlV(IntPtr targetWindow);
}

public class InputSimulationService : IInputSimulationService
{
    public void SimulateCtrlV()
    {
        var inputs = new NativeMethods.INPUT[4];

        inputs[0].type = NativeMethods.INPUT_KEYBOARD;
        inputs[0].U.ki.wVk = NativeMethods.VK_CONTROL;

        inputs[1].type = NativeMethods.INPUT_KEYBOARD;
        inputs[1].U.ki.wVk = NativeMethods.VK_V;

        inputs[2].type = NativeMethods.INPUT_KEYBOARD;
        inputs[2].U.ki.wVk = NativeMethods.VK_V;
        inputs[2].U.ki.dwFlags = NativeMethods.KEYEVENTF_KEYUP;

        inputs[3].type = NativeMethods.INPUT_KEYBOARD;
        inputs[3].U.ki.wVk = NativeMethods.VK_CONTROL;
        inputs[3].U.ki.dwFlags = NativeMethods.KEYEVENTF_KEYUP;

        NativeMethods.SendInput(4, inputs, Marshal.SizeOf<NativeMethods.INPUT>());
    }

    /// <summary>
    /// Post Ctrl+V keystrokes directly to target window — no focus switch needed.
    /// </summary>
    public void PostCtrlV(IntPtr targetWindow)
    {
        // Build lParam for WM_KEYDOWN/KEYUP
        // Bits 0-15: repeat count (1)
        // Bits 16-23: scan code
        // Bit 29: context code (0)
        // Bit 30: previous key state (0=up, 1=down)
        // Bit 31: transition (0=press, 1=release)

        uint scanCtrl = NativeMethods.MapVirtualKey(NativeMethods.VK_CONTROL, 0);
        uint scanV = NativeMethods.MapVirtualKey(NativeMethods.VK_V, 0);

        IntPtr lParamCtrlDown = (IntPtr)(1 | (scanCtrl << 16));
        IntPtr lParamCtrlUp   = (IntPtr)(1 | (scanCtrl << 16) | (1u << 30) | (1u << 31));
        IntPtr lParamVDown    = (IntPtr)(1 | (scanV << 16));
        IntPtr lParamVUp      = (IntPtr)(1 | (scanV << 16) | (1u << 30) | (1u << 31));

        NativeMethods.PostMessage(targetWindow, NativeMethods.WM_KEYDOWN, (IntPtr)NativeMethods.VK_CONTROL, lParamCtrlDown);
        NativeMethods.PostMessage(targetWindow, NativeMethods.WM_KEYDOWN, (IntPtr)NativeMethods.VK_V, lParamVDown);
        NativeMethods.PostMessage(targetWindow, NativeMethods.WM_KEYUP, (IntPtr)NativeMethods.VK_V, lParamVUp);
        NativeMethods.PostMessage(targetWindow, NativeMethods.WM_KEYUP, (IntPtr)NativeMethods.VK_CONTROL, lParamCtrlUp);
    }
}
