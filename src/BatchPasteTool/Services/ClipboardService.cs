namespace BatchPasteTool.Services;

public interface IClipboardService
{
    void SetText(string text);
}

public class ClipboardService : IClipboardService
{
    public void SetText(string text)
    {
        System.Windows.Clipboard.SetText(text, System.Windows.TextDataFormat.UnicodeText);
    }
}
