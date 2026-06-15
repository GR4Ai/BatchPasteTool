# BatchPasteTool

A lightweight Windows desktop utility for batch text pasting. Store multiple text snippets and paste them anywhere with a single click.

## Features

- **Batch Paste**: Maintain multiple text entries, each with its own Send button. Clicking "Send" copies the text to the clipboard and pastes it at the cursor position (simulates Ctrl+V) in any application.
- **Always-on-Top**: Pin the window to stay above all other windows for quick access.
- **Transparency Control**: Adjust the window transparency from fully opaque to 80% transparent using the slider.
- **Undo Support**: Undo any operation — text changes, item additions/deletions, and clear operations. Also accessible via Ctrl+Z.
- **Memory**: All text content is automatically saved on exit and restored when the program reopens.
- **Responsive Layout**: Resize the window freely; all components adapt to the new size.
- **Lightweight**: Built with native Win32 API for minimal memory footprint and fast execution.

## Screenshots

*(Refer to the UI mockup images in the project folder: 图片0.png and 图片00.png)*

## System Requirements

- Windows 7 or later (64-bit recommended)
- No additional runtime dependencies (statically linked)

## Installation

### Option 1: Download Pre-built

Download `BatchPasteTool.exe` and place it in any folder. Make sure the `resources/` folder is in the same directory as the executable.

### Option 2: Build from Source

#### Using MSVC (Visual Studio)

1. Open "Developer Command Prompt for VS"
2. Navigate to the project directory
3. Run: `build.bat`

#### Using MinGW-w64

1. Ensure `g++.exe` and `windres.exe` are in your PATH
2. Navigate to the project directory
3. Run: `build.bat`

#### Manual Build

**MSVC:**
```
rc BatchPasteTool.rc
cl /O1 /EHsc /Fe:BatchPasteTool.exe src\main.cpp BatchPasteTool.res /link gdiplus.lib comctl32.lib
```

**MinGW-w64:**
```
g++ -O2 -s -static -mwindows -o BatchPasteTool.exe src\main.cpp -lgdiplus -lcomctl32
```

## Project Structure

```
BatchPasteTool/
├── src/
│   ├── main.cpp          # Main source code
│   └── resource.h        # Resource definitions
├── resources/            # Image resources (PNG files)
│   ├── app_icon.png      # Application icon
│   ├── add.png           # Add button icon
│   ├── delete.png        # Delete button icon
│   ├── pin.png           # Pin button icon
│   ├── transparency.png  # Transparency button icon
│   ├── undo.png          # Undo button icon
│   ├── slider_bg.png     # Slider background
│   └── slider_parts.png  # Slider components
├── BatchPasteTool.rc     # Resource script
├── BatchPasteTool.manifest # Application manifest
├── build.bat             # Build script
└── README.md             # This file
```

## Usage Guide

### Interface Layout

```
┌─────────────────────────────────────────────────────────┐
│ BatchPasteTool    [🔆 ═══○═══] [📌]   [_] [□] [X]    │  ← Title bar
├──────────────────────────────────────────────┬──────────┤
│ ┌─────────────────────────────────┐ [✕][Send]│  ▲      │
│ │ Enter text here...              │          │  █      │
│ │─────────────────────────────────│          │  █      │  ← Scrollable
│ │ More text...                    │ [✕][Send]│  █      │     item list
│ │─────────────────────────────────│          │  █      │
│ │ Another entry...                │ [✕][Send]│  █      │
│ └─────────────────────────────────┘          │  ▼      │
├──────────────────────────────────────────────┴──────────┤
│ [+] [-] [↩]                        [Clear All] [Clear]  │  ← Bottom bar
└─────────────────────────────────────────────────────────┘
```

### Controls

| Control | Description |
|---------|-------------|
| **Title Bar** | Drag to move the window. |
| **Pin Button** (📌) | Toggle always-on-top mode. Highlighted border when active. |
| **Transparency Icon** (🔆) | Toggle between opaque and semi-transparent. |
| **Transparency Slider** | Drag the circle to adjust window opacity. Left = opaque, Right = 80% transparent. |
| **Minimize / Maximize / Close** | Standard window control buttons. |
| **Text Field** | Type or paste text to be sent. |
| **✕ (Clear)** | Clear the text in this item. |
| **Send** | Copy text to clipboard and paste at cursor position. |
| **+ (Add)** | Add a new empty item below the last one. |
| **− (Delete)** | Delete the last item (at least one item always remains). |
| **↩ (Undo)** | Undo the last operation. Also Ctrl+Z. |
| **Clear All** | Remove all items except the first one, and clear its text. |
| **Clear Texts** | Clear the text content of all items without removing any items. |

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo last operation |
| `Ctrl+N` | Add new item |
| `Ctrl+Shift+S` | Send all items (paste all text sequentially) |

### Workflow Example

1. Launch **BatchPasteTool**.
2. Click **+** to add text entries as needed.
3. Type your text snippets into each field (e.g., email templates, code snippets, frequently used phrases).
4. Click the **Pin** button if you want the tool to stay on top of other windows.
5. In your target application (Notepad, browser, Word, etc.), place the cursor where you want to paste.
6. Click the **Send** button next to the desired text entry.
7. The text will be pasted at the cursor position.

## Configuration

The program saves its state to `BatchPasteTool.ini` in the same directory as the executable. The file stores:
- All text content
- Window position and size
- Pin state
- Transparency level

This file is automatically saved every 3 seconds and when the program exits.

## Technical Details

- **Language**: C++17
- **UI Framework**: Win32 API + GDI+ (for PNG rendering)
- **Dependencies**: None beyond standard Windows libraries (gdiplus.dll, comctl32.dll)
- **Memory Usage**: ~5-8 MB typical
- **Executable Size**: ~200-400 KB (statically linked)

## Troubleshooting

**Q: The paste doesn't work in my target application.**
A: Make sure the cursor is placed in a text-editable area before clicking Send. Some applications (e.g., certain secure input fields) may block simulated keyboard input.

**Q: The window is too transparent and I can't see it.**
A: Restart the application — the default opacity will be restored. Or delete `BatchPasteTool.ini` to reset all settings.

**Q: Windows SmartScreen warns about the executable.**
A: This is normal for newly compiled applications. The program is safe. You can click "More info" → "Run anyway".

## License

This project is provided as-is for personal and professional use.
