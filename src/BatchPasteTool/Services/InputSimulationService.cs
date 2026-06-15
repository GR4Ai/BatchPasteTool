using System.Runtime.InteropServices;
using BatchPasteTool.Helpers;

namespace BatchPasteTool.Services;

public interface IInputSimulationService
{
    void SimulateCtrlV();
}

public class InputSimulationService : IInputSimulationService
{
    public void SimulateCtrlV()
    {
        var inputs = new NativeMethods.INPUT[4];

        // Ctrl down
        inputs[0].type = NativeMethods.INPUT_KEYBOARD;
        inputs[0].U.ki.wVk = NativeMethods.VK_CONTROL;

        // V down
        inputs[1].type = NativeMethods.INPUT_KEYBOARD;
        inputs[1].U.ki.wVk = NativeMethods.VK_V;

        // V up
        inputs[2].type = NativeMethods.INPUT_KEYBOARD;
        inputs[2].U.ki.wVk = NativeMethods.VK_V;
        inputs[2].U.ki.dwFlags = NativeMethods.KEYEVENTF_KEYUP;

        // Ctrl up
        inputs[3].type = NativeMethods.INPUT_KEYBOARD;
        inputs[3].U.ki.wVk = NativeMethods.VK_CONTROL;
        inputs[3].U.ki.dwFlags = NativeMethods.KEYEVENTF_KEYUP;

        NativeMethods.SendInput(4, inputs, Marshal.SizeOf<NativeMethods.INPUT>());
    }
}
