using System.IO;
using System.Text;
using System.Text.Json;
using BatchPasteTool.Models;

namespace BatchPasteTool.Services;

public interface IConfigService
{
    AppSettings Load();
    void Save(AppSettings settings);
}

public class ConfigService : IConfigService
{
    private readonly string _filePath;

    public ConfigService()
    {
        _filePath = Path.Combine(AppContext.BaseDirectory, Helpers.Constants.ConfigFile);
    }

    public AppSettings Load()
    {
        if (!File.Exists(_filePath))
            return NewDefault();

        try
        {
            string json = File.ReadAllText(_filePath, Encoding.UTF8);
            var settings = JsonSerializer.Deserialize<AppSettings>(json);
            if (settings == null) return NewDefault();

            // Clamp values
            if (settings.Transparency < Helpers.Constants.TranspMin)
                settings.Transparency = Helpers.Constants.TranspMin;
            if (settings.Transparency > Helpers.Constants.TranspMax)
                settings.Transparency = Helpers.Constants.TranspMax;
            if (settings.ItemTexts.Count < 1) settings.ItemTexts.Add("");
            if (settings.ItemTexts.Count > Helpers.Constants.MaxItems)
                settings.ItemTexts = settings.ItemTexts.Take(Helpers.Constants.MaxItems).ToList();
            if (settings.WinW < Helpers.Constants.MinWinW) settings.WinW = Helpers.Constants.MinWinW;
            if (settings.WinH < Helpers.Constants.MinWinH) settings.WinH = Helpers.Constants.MinWinH;

            return settings;
        }
        catch
        {
            return NewDefault();
        }
    }

    public void Save(AppSettings settings)
    {
        try
        {
            string dir = Path.GetDirectoryName(_filePath)!;
            if (!Directory.Exists(dir)) Directory.CreateDirectory(dir);
            var options = new JsonSerializerOptions { WriteIndented = true };
            string json = JsonSerializer.Serialize(settings, options);
            File.WriteAllText(_filePath, json, Encoding.UTF8);
        }
        catch
        {
            // Silently fail — save is non-critical
        }
    }

    private static AppSettings NewDefault() => new()
    {
        ItemTexts = new List<string> { "" },
        Transparency = 255,
        WinW = 620,
        WinH = 500
    };
}
