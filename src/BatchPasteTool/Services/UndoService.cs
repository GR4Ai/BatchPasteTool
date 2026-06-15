using BatchPasteTool.Models;

namespace BatchPasteTool.Services;

public interface IUndoService
{
    void Push(UndoEntry entry);
    UndoEntry? Pop();
    bool CanUndo { get; }
    void Clear();
}

public class UndoService : IUndoService
{
    private readonly Stack<UndoEntry> _stack = new();
    private const int MaxEntries = 200;

    public bool CanUndo => _stack.Count > 0;

    public void Push(UndoEntry entry)
    {
        _stack.Push(entry);
        // Trim if over 200 entries
        if (_stack.Count > MaxEntries)
        {
            var temp = _stack.ToArray();
            _stack.Clear();
            foreach (var e in temp.Take(100).Reverse())
                _stack.Push(e);
        }
    }

    public UndoEntry? Pop()
    {
        return _stack.Count > 0 ? _stack.Pop() : null;
    }

    public void Clear()
    {
        _stack.Clear();
    }
}
