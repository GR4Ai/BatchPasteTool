using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace BatchPasteTool.ViewModels;

/// <summary>
/// ViewModel for a single paste item row.
/// </summary>
public class PasteItemViewModel : INotifyPropertyChanged
{
    private string _text = string.Empty;
    private string _textOnFocus = string.Empty;

    public int Index { get; set; }

    public string Text
    {
        get => _text;
        set
        {
            if (_text != value)
            {
                _text = value;
                OnPropertyChanged();
            }
        }
    }

    /// <summary>
    /// Save current text when the TextBox gets focus (for undo tracking).
    /// </summary>
    public void SaveFocusText()
    {
        _textOnFocus = _text;
    }

    /// <summary>
    /// Get the text that was present when focus was gained,
    /// and return whether it differs from the current text.
    /// </summary>
    public bool GetTextChange(out string oldText, out string newText)
    {
        oldText = _textOnFocus;
        newText = _text;
        return oldText != newText;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    protected void OnPropertyChanged([CallerMemberName] string? name = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}
