namespace BatchPasteTool.Models;

/// <summary>
/// Data model for a single paste item (one text row).
/// </summary>
public class PasteItem
{
    public int Id { get; set; }
    public string Text { get; set; } = string.Empty;
}
