using System.Windows;
using BatchPasteTool.Services;
using BatchPasteTool.ViewModels;
using BatchPasteTool.Views;

namespace BatchPasteTool;

public partial class App : Application
{
    private void OnStartup(object sender, StartupEventArgs e)
    {
        // Manual DI
        var clipboard = new ClipboardService();
        var inputSim = new InputSimulationService();
        var config = new ConfigService();
        var foreground = new ForegroundWindowService();
        var undo = new UndoService();

        var vm = new MainViewModel(clipboard, inputSim, config, foreground, undo);

        var window = new MainWindow
        {
            DataContext = vm
        };

        window.Show();
    }
}
