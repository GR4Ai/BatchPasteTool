namespace BatchPasteTool.Models;

/// <summary>
/// JSON-serializable application settings for persistence.
/// </summary>
public class AppSettings
{
    public bool Pinned { get; set; }
    public int Transparency { get; set; } = 255; // 255 = opaque
    public int WinX { get; set; } = int.MinValue; // CW_USEDEFAULT sentinel
    public int WinY { get; set; } = int.MinValue;
    public int WinW { get; set; } = 620;
    public int WinH { get; set; } = 500;
    public List<string> ItemTexts { get; set; } = new() { "" };
}
