namespace BatchPasteTool.Models;

public class UndoEntry
{
    public UndoType Type { get; set; }
    public int ItemIndex { get; set; } = -1;
    public string? OldText { get; set; }
    public string? NewText { get; set; }
    public List<string>? OldItemTexts { get; set; }
}
