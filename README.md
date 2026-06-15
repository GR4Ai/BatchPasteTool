# BatchPasteTool

A lightweight Windows desktop utility for batch text pasting. Store multiple text snippets and paste them anywhere with a single click.

Built with **C# WPF (.NET 8.0)** — borderless custom window with grayscale UI design.

> 📖 [中文版本](README_CN.md)

## Features

- **Batch Paste**: Maintain multiple text entries, each with its own Send button. Clicking "Send" copies the text to the clipboard and pastes it at the cursor position (simulates Ctrl+V) in any application.
- **Always-on-Top**: Pin the window to stay above all other windows for quick access.
- **Transparency Control**: Adjust the window transparency from fully opaque to 80% transparent using the slider.
- **Undo Support**: Undo any operation — text changes, item additions/deletions, and clear operations. Also accessible via Ctrl+Z.
- **Persistence**: All text content, window position/size, pin state, and transparency level are automatically saved and restored on restart.
- **Responsive Layout**: Resize the window freely; the transparency slider stretches with the window width.
- **Virtualized Item List**: Efficiently handles many items using WPF virtualizing stack panel.

## System Requirements

- Windows 10 or later (64-bit)
- [.NET 8.0 Desktop Runtime](https://dotnet.microsoft.com/en-us/download/dotnet/8.0) (or self-contained publish)

## Installation

### Option 1: Download Pre-built

Choose the version that fits your needs:

| Version | File | Size | Description |
|---------|------|------|-------------|
| **Portable** | [BatchPasteTool.exe](https://github.com/GR4Ai/BatchPasteTool/releases/latest/download/BatchPasteTool.exe) | ~162 MB | Single EXE, no installation, no .NET runtime required. Just run it. |
| **Installer** | [BatchPasteTool-Setup.exe](https://github.com/GR4Ai/BatchPasteTool/releases/latest/download/BatchPasteTool-Setup.exe) | ~67 MB | Self-extracting installer. Installs to `%LocalAppData%\Programs\BatchPasteTool`, creates Start Menu and Desktop shortcuts, includes uninstaller. |

> **Note:** The portable version bundles the full .NET runtime (larger size but zero dependencies). The installer version uses ZIP compression for a smaller download while still being self-contained.

### Option 2: Build from Source

```bash
# Regular build (requires .NET 8 runtime)
cd src/BatchPasteTool
dotnet build -c Release
```

#### Portable (self-contained single-file)

```bash
dotnet publish -c Release -r win-x64 --self-contained -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o publish
```

Produces `publish/BatchPasteTool.exe` (~162 MB).

#### Installer

```bash
# 1. Publish multi-file
dotnet publish -c Release -r win-x64 --self-contained -o publish-installer

# 2. Build SFX stub
dotnet publish -c Release -o installer/SfxStub/publish installer/SfxStub/SfxStub.csproj

# 3. Merge into installer EXE (requires PowerShell)
powershell -File scripts/merge-installer.ps1
```

Produces `installer/BatchPasteTool-Setup.exe` (~67 MB).

## Project Structure

```
BatchPasteTool/
├── BatchPasteTool.sln
├── src/
│   └── BatchPasteTool/
│       ├── BatchPasteTool.csproj
│       ├── App.xaml / App.xaml.cs
│       ├── Models/
│       │   ├── PasteItem.cs
│       │   ├── UndoEntry.cs / UndoType.cs
│       │   └── AppSettings.cs
│       ├── ViewModels/
│       │   ├── MainViewModel.cs
│       │   ├── PasteItemViewModel.cs
│       │   └── RelayCommand.cs
│       ├── Views/
│       │   ├── MainWindow.xaml / .cs
│       │   └── Converters/
│       ├── Services/
│       │   ├── ClipboardService.cs
│       │   ├── InputSimulationService.cs
│       │   ├── ConfigService.cs
│       │   ├── ForegroundWindowService.cs
│       │   └── UndoService.cs
│       ├── Helpers/
│       │   ├── NativeMethods.cs
│       │   └── Constants.cs
│       └── Resources/
│           ├── app_icon.ico / app_icon.png
│           ├── add.png
│           ├── delete.png
│           ├── pin.png / pin_filled.png
│           ├── transparency.png
│           └── undo.png
├── app.manifest
├── installer/
│   ├── install.ps1                   # Post-install script
│   └── SfxStub/                      # Self-extracting stub source
├── reference-images/                 # UI reference screenshots
├── legacy/                           # Original C++ source files
├── README.md
└── README_CN.md
```

## Usage Guide

### Interface Layout

```
┌──────────────────────────────────────────────────────────────┐
│ BatchPasteTool [📌] [🔆] [═══════○═══════]   [_] [□] [X]    │  ← Title bar (32px)
├──────────────────────────────────────────────────┬───────────┤
│ ┌──────────────────────────────────┐ [✕] [Send]  │  ▲        │
│ │ Enter text here...               │              │  █        │
│ │──────────────────────────────────│              │  █        │  ← Scrollable
│ │ More text...                     │ [✕] [Send]   │  █        │     item list
│ │──────────────────────────────────│              │  █        │
│ │ Another entry...                 │ [✕] [Send]   │  █        │
│ └──────────────────────────────────┘              │  ▼        │
├──────────────────────────────────────────────────┴───────────┤
│ [+] [-] [↩]                             [Clear All] [Clear]  │  ← Bottom bar (42px)
└──────────────────────────────────────────────────────────────┘
```

### Controls

| Control | Description |
|---------|-------------|
| **Title Bar** | Drag to move the window. |
| **Pin Button** (📌) | Toggle always-on-top mode. Filled icon when active. |
| **Transparency Icon** (🔆) | Toggle between opaque and semi-transparent. |
| **Transparency Slider** | Drag to adjust window opacity. Stretches with window width. |
| **Minimize / Maximize / Close** | Standard window control buttons. Hover to highlight. |
| **Text Field** | Type or paste text to be sent. Font: Segoe UI 18px. |
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
| `Ctrl+Shift+S` | Send all items sequentially |

### Workflow Example

1. Launch **BatchPasteTool**.
2. Click **+** to add text entries as needed.
3. Type or paste text snippets into each field.
4. Toggle **Pin** to keep the window on top.
5. Adjust **Transparency** with the slider if desired.
6. Click **Send** next to any entry to paste it into the target application.
7. Use **Ctrl+Z** to undo mistakes.

## Configuration

State is saved to `BatchPasteTool.json` in the executable directory. Saved data includes:
- All text content
- Window position and size
- Pin state
- Transparency level

Auto-saved every 3 seconds and on exit.

## Technical Details

- **Language**: C# 12
- **Framework**: WPF (.NET 8.0)
- **Architecture**: MVVM (hand-rolled, no framework)
- **Windowing**: Borderless custom window, `AllowsTransparency=True`, custom `WM_NCHITTEST` for 8-direction resize
- **Input Simulation**: Win32 `SendInput` API (Ctrl+V)
- **Serialization**: System.Text.Json
- **No third-party NuGet dependencies**

## License

This project is provided as-is for personal and professional use.
