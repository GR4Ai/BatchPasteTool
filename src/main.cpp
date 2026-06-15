/******************************************************************************
 * BatchPasteTool - A lightweight Windows desktop tool for batch text pasting
 *
 * Features:
 *   - Custom title bar with always-on-top toggle & transparency slider
 *   - Multiple text items, each with Send (paste) and Clear buttons
 *   - Undo support, persistent state, responsive layout
 *
 * Build (MSVC):
 *   cl src\main.cpp /Fe:BatchPasteTool.exe /O1 /EHsc /link gdiplus.lib comctl32.lib
 *
 * Build (MinGW-w64):
 *   g++ src\main.cpp -o BatchPasteTool.exe -O2 -s -static -lgdiplus -lcomctl32 -mwindows
 ******************************************************************************/

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <algorithm>
#include <fstream>
#include <stack>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

// ============================================================================
//  CONSTANTS
// ============================================================================

#define APP_NAME       L"BatchPasteTool"
#define APP_CLASS      L"BatchPasteToolWnd"
#define CONFIG_FILE    L"BatchPasteTool.ini"

#define TITLE_H        36     // Title bar height
#define BOTTOM_H       42     // Bottom bar height
#define ITEM_H         38     // Per-item height
#define ITEM_GAP       4      // Gap between items
#define ICON_W         24     // Icon button width
#define ICON_H         24     // Icon button height
#define SCROLLBAR_W    16     // Scrollbar width
#define SLIDER_W       140    // Transparency slider total width
#define SLIDER_H       18     // Transparency slider height
#define BTN_W          48     // Text button width in bottom bar
#define BTN_H          28     // Text button height
#define WINBTN_W       36     // Window control button width
#define WINBTN_H       28     // Window control button height

#define MIN_WIN_W      420    // Minimum window width
#define MIN_WIN_H      220    // Minimum window height

#define IDT_CHECK_FG   100    // Timer: foreground window check
#define IDT_AUTOSAVE   101    // Timer: auto-save

// Child control IDs
#define CID_ITEM_BASE  1000   // Base ID for item edit controls

// ============================================================================
//  COLORS  (White / Light theme)
// ============================================================================

#define RGB_TITLE_BG       RGB(240, 240, 240)   // Title bar: light gray
#define RGB_TITLE_TEXT     RGB( 32,  32,  32)   // Title text: dark
#define RGB_BOTTOM_BG      RGB(240, 240, 240)   // Bottom bar: light gray
#define RGB_CONTENT_BG     RGB(255, 255, 255)   // Content area: white
#define RGB_ITEM_BG        RGB(248, 248, 248)   // Item background: off-white
#define RGB_SEPARATOR      RGB(220, 220, 220)   // Separator line: light gray
#define RGB_BTN_TEXT       RGB( 60,  60,  60)   // Button text: dark gray
#define RGB_BTN_HOVER      RGB(200, 220, 240)   // Button hover: light blue
#define RGB_SLIDER_TRACK   RGB(210, 210, 210)   // Slider track: light gray
#define RGB_SLIDER_FILL    RGB( 66, 133, 244)   // Slider fill: Google blue
#define RGB_SLIDER_EMPTY   RGB(230, 230, 230)   // Slider empty: lighter gray
#define RGB_SLIDER_THUMB   RGB( 66, 133, 244)   // Slider thumb: blue
#define RGB_CLOSE_HOVER    RGB(232,  78,  64)   // Close button hover: red

// ============================================================================
//  STRUCTURES
// ============================================================================

// Types of undoable actions
enum class UndoType {
    TextChanged,
    ItemAdded,
    ItemDeleted,
    AllItemsCleared,
    AllTextsCleared
};

// Single undo entry
struct UndoEntry {
    UndoType type;
    int     itemIndex;           // Index of affected item (-1 for all)
    std::wstring oldText;        // Previous text (for TextChanged)
    std::wstring newText;        // New text (for TextChanged)
    std::vector<std::wstring> oldItemTexts; // For AllCleared / ItemDeleted
};

// One paste item (column)
struct PasteItem {
    HWND  hEdit;      // Edit control handle
    HWND  hClearBtn;  // Clear button handle
    HWND  hSendBtn;   // Send button handle
    RECT  rcEdit;     // Current edit rect
    RECT  rcClear;    // Current clear button rect
    RECT  rcSend;     // Current send button rect
};

// ============================================================================
//  GLOBAL STATE
// ============================================================================

static HWND   g_hWnd          = nullptr;   // Main window
static HWND   g_hContentWnd   = nullptr;   // Content container (static)
static HWND   g_hScrollbar    = nullptr;   // Vertical scrollbar
static HINSTANCE g_hInst      = nullptr;   // App instance

static std::vector<PasteItem> g_items;       // All paste items
static std::stack<UndoEntry>  g_undoStack;   // Undo history

static HWND   g_hTargetWnd    = nullptr;    // Last known target (non-app) foreground window
static bool   g_bPinned       = false;      // Always-on-top state
static int    g_iTransparency = 255;        // 255=opaque, 51=80% transparent
static bool   g_bDragging     = false;      // Slider drag in progress
static int    g_iScrollPos    = 0;          // Current scroll position
static bool   g_bCloseHover   = false;      // Close button hover state
static bool   g_bMinHover     = false;      // Min button hover state
static bool   g_bMaxHover     = false;      // Max button hover state
static bool   g_bPinHover     = false;      // Pin button hover state
static bool   g_bTranspHover  = false;      // Transparency icon hover state
static bool   g_bAddHover     = false;      // Add button hover state
static bool   g_bDelHover     = false;      // Delete button hover state
static bool   g_bUndoHover    = false;      // Undo button hover state
static bool   g_bClrAllHover  = false;      // Clear-all button hover state
static bool   g_bClrTxtHover  = false;      // Clear-texts button hover state
static bool   g_bSliderHover  = false;      // Slider hover state
static int    g_iHoverItem    = -1;         // Which item's Send btn is hovered (-1=none)
static int    g_iHoverClear   = -1;         // Which item's Clear btn is hovered

// Saved GDI+ bitmaps – loaded from PNG at startup
static Gdiplus::Bitmap* g_pBmpAdd       = nullptr;
static Gdiplus::Bitmap* g_pBmpDelete    = nullptr;
static Gdiplus::Bitmap* g_pBmpPin       = nullptr;
static Gdiplus::Bitmap* g_pBmpTransp    = nullptr;
static Gdiplus::Bitmap* g_pBmpUndo      = nullptr;
static Gdiplus::Bitmap* g_pBmpSliderBg  = nullptr;
static Gdiplus::Bitmap* g_pBmpAppIcon   = nullptr;

// Cached GDI+ brushes / pens
static Gdiplus::SolidBrush* g_pBrTitleBg   = nullptr;
static Gdiplus::SolidBrush* g_pBrContentBg = nullptr;
static Gdiplus::SolidBrush* g_pBrItemBg    = nullptr;
static Gdiplus::SolidBrush* g_pBrBtnText   = nullptr;
static Gdiplus::SolidBrush* g_pBrBtnHover  = nullptr;
static Gdiplus::SolidBrush* g_pBrCloseHov  = nullptr;
static Gdiplus::SolidBrush* g_pBrSliderFill= nullptr;
static Gdiplus::SolidBrush* g_pBrSliderEmp = nullptr;
static Gdiplus::SolidBrush* g_pBrSliderThumb=nullptr;
static Gdiplus::Font*       g_pFontTitle   = nullptr;
static Gdiplus::Font*       g_pFontBtn     = nullptr;

// Layout rectangles (pixel coords, recalculated on WM_SIZE)
static RECT g_rcTitleBar,  g_rcName;
static RECT g_rcPinBtn,    g_rcTranspIcon, g_rcTranspSlider;
static RECT g_rcMinBtn,    g_rcMaxBtn,     g_rcCloseBtn;
static RECT g_rcContent,   g_rcScrollbar;
static RECT g_rcBottom;
static RECT g_rcAddBtn,    g_rcDelBtn,     g_rcUndoBtn;
static RECT g_rcClearAll,  g_rcClearTexts;

// Per-button hover tracking rects for direct comparison
struct HoverZone {
    RECT rc;
    bool* pHover;
};

// ============================================================================
//  FORWARD DECLARATIONS
// ============================================================================

LRESULT CALLBACK MainWndProc     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
void     UpdateLayout             ();
void     RebuildItems         ();
void     RepositionItem       (int idx);
void     SyncScrollbar        ();
void     OnPaint              (HWND, HDC);
void     DrawTitleBar         (Gdiplus::Graphics& g);
void     DrawContentArea      (Gdiplus::Graphics& g);
void     DrawBottomBar        (Gdiplus::Graphics& g);
void     DrawSlider           (Gdiplus::Graphics& g);
bool     LoadPngFile           (const wchar_t* name, Gdiplus::Bitmap** ppBmp);
void     LoadAllPngs           ();
void     FreeAllPngs           ();
void     InitGdiPlusCache     ();
void     FreeGdiPlusCache     ();
void     OnAddItem            ();
void     OnDeleteItem         ();
void     OnUndo               ();
void     OnClearAllItems      ();
void     OnClearAllTexts      ();
void     OnSend               (int idx);
void     OnClearItem          (int idx);
void     PushUndo             (const UndoEntry& e);
void     SaveState            ();
void     LoadState            ();
void     SimulatePaste        ();
void     SetClipText          (const std::wstring& text);
void     ApplyTransparency    ();
void     ApplyPin             ();
void     GetConfigPath        (wchar_t* buf, int len);
LRESULT  HandleMouseMove      (HWND, int x, int y);
void     UpdateAllHovers      (int x, int y);
bool     PtInRc               (const RECT& rc, int x, int y);
Gdiplus::Bitmap* LoadPngResource(const wchar_t* name);
void     CreateItemControls   (int idx);
void     DestroyItemControls  (int idx);

// ============================================================================
//  UTILITY
// ============================================================================

bool PtInRc(const RECT& rc, int x, int y) {
    return x >= rc.left && x < rc.right && y >= rc.top && y < rc.bottom;
}

inline Gdiplus::Rect ToGdipRect(const RECT& rc) {
    return Gdiplus::Rect((INT)rc.left, (INT)rc.top,
                         (INT)(rc.right - rc.left),
                         (INT)(rc.bottom - rc.top));
}

void GetConfigPath(wchar_t* buf, int len) {
    // Store config in the same directory as the EXE
    GetModuleFileNameW(nullptr, buf, len);
    wchar_t* pLast = wcsrchr(buf, L'\\');
    if (pLast) *(pLast + 1) = 0;
    wcscat_s(buf, len, CONFIG_FILE);
}

// ============================================================================
//  IMAGE LOADING (GDI+)
// ============================================================================

bool LoadPngFile(const wchar_t* name, Gdiplus::Bitmap** ppBmp) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wchar_t* p = wcsrchr(path, L'\\');
    if (p) *(p + 1) = 0;
    wcscat_s(path, L"resources\\");
    wcscat_s(path, name);
    *ppBmp = Gdiplus::Bitmap::FromFile(path);
    return (*ppBmp != nullptr && (*ppBmp)->GetLastStatus() == Gdiplus::Ok);
}

void LoadAllPngs() {
    LoadPngFile(L"add.png",            &g_pBmpAdd);
    LoadPngFile(L"delete.png",         &g_pBmpDelete);
    LoadPngFile(L"pin.png",            &g_pBmpPin);
    LoadPngFile(L"transparency.png",   &g_pBmpTransp);
    LoadPngFile(L"undo.png",           &g_pBmpUndo);
    LoadPngFile(L"slider_bg.png",      &g_pBmpSliderBg);
    LoadPngFile(L"app_icon.png",       &g_pBmpAppIcon);
}

void FreeAllPngs() {
    #define SAFE_DEL(p) if(p) { delete p; p=nullptr; }
    SAFE_DEL(g_pBmpAdd)
    SAFE_DEL(g_pBmpDelete)
    SAFE_DEL(g_pBmpPin)
    SAFE_DEL(g_pBmpTransp)
    SAFE_DEL(g_pBmpUndo)
    SAFE_DEL(g_pBmpSliderBg)
    SAFE_DEL(g_pBmpAppIcon)
    #undef SAFE_DEL
}

void InitGdiPlusCache() {
    g_pBrTitleBg    = new Gdiplus::SolidBrush(Gdiplus::Color(240, 240, 240));  // light gray
    g_pBrContentBg  = new Gdiplus::SolidBrush(Gdiplus::Color(255, 255, 255));  // white
    g_pBrItemBg     = new Gdiplus::SolidBrush(Gdiplus::Color(248, 248, 248));  // off-white
    g_pBrBtnText    = new Gdiplus::SolidBrush(Gdiplus::Color( 60,  60,  60));  // dark gray
    g_pBrBtnHover   = new Gdiplus::SolidBrush(Gdiplus::Color(200, 220, 240));  // light blue
    g_pBrCloseHov   = new Gdiplus::SolidBrush(Gdiplus::Color(232,  78,  64));  // red
    g_pBrSliderFill = new Gdiplus::SolidBrush(Gdiplus::Color( 66, 133, 244));  // blue
    g_pBrSliderEmp  = new Gdiplus::SolidBrush(Gdiplus::Color(230, 230, 230));  // light gray
    g_pBrSliderThumb= new Gdiplus::SolidBrush(Gdiplus::Color( 66, 133, 244));  // blue thumb
    g_pFontTitle    = new Gdiplus::Font(L"Segoe UI", 10.0f, Gdiplus::FontStyleBold);
    g_pFontBtn      = new Gdiplus::Font(L"Segoe UI", 9.0f);
}

void FreeGdiPlusCache() {
    #define SAFE_DEL(p) if(p) { delete p; p=nullptr; }
    SAFE_DEL(g_pBrTitleBg)
    SAFE_DEL(g_pBrContentBg)
    SAFE_DEL(g_pBrItemBg)
    SAFE_DEL(g_pBrBtnText)
    SAFE_DEL(g_pBrBtnHover)
    SAFE_DEL(g_pBrCloseHov)
    SAFE_DEL(g_pBrSliderFill)
    SAFE_DEL(g_pBrSliderEmp)
    SAFE_DEL(g_pBrSliderThumb)
    SAFE_DEL(g_pFontTitle)
    SAFE_DEL(g_pFontBtn)
    #undef SAFE_DEL
}

// ============================================================================
//  LAYOUT CALCULATION
// ============================================================================

void UpdateLayout() {
    RECT rc; GetClientRect(g_hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // --- Title bar (top) ---
    g_rcTitleBar   = {0, 0, w, TITLE_H};

    // Name: left side of title bar
    g_rcName       = {10, 0, 180, TITLE_H};

    // Window control buttons: right-aligned
    g_rcCloseBtn   = {w - WINBTN_W, 4, w, TITLE_H};
    g_rcMaxBtn     = {w - WINBTN_W * 2, 4, w - WINBTN_W, TITLE_H};
    g_rcMinBtn     = {w - WINBTN_W * 3, 4, w - WINBTN_W * 2, TITLE_H};

    // Pin button
    g_rcPinBtn     = {w - WINBTN_W * 3 - ICON_W - 10, (TITLE_H - ICON_H) / 2,
                      w - WINBTN_W * 3 - 10, (TITLE_H - ICON_H) / 2 + ICON_H};

    // Transparency icon
    g_rcTranspIcon = {g_rcPinBtn.left - ICON_W - 10, (TITLE_H - ICON_H) / 2,
                      g_rcPinBtn.left - 10, (TITLE_H - ICON_H) / 2 + ICON_H};

    // Transparency slider (between transparency icon and pin button, but left of transp icon)
    int sliderLeft  = g_rcTranspIcon.left - SLIDER_W - 8;
    g_rcTranspSlider = {sliderLeft, (TITLE_H - SLIDER_H) / 2,
                        g_rcTranspIcon.left - 8, (TITLE_H - SLIDER_H) / 2 + SLIDER_H};

    // --- Bottom bar ---
    g_rcBottom     = {0, h - BOTTOM_H, w, h};

    // Left buttons
    int btnY = g_rcBottom.top + (BOTTOM_H - ICON_H) / 2;
    g_rcAddBtn     = {10, btnY, 10 + ICON_W, btnY + ICON_H};
    g_rcDelBtn     = {g_rcAddBtn.right + 8, btnY, g_rcAddBtn.right + 8 + ICON_W, btnY + ICON_H};
    g_rcUndoBtn    = {g_rcDelBtn.right + 8, btnY, g_rcDelBtn.right + 8 + ICON_W, btnY + ICON_H};

    // Right buttons
    int clrY = g_rcBottom.top + (BOTTOM_H - BTN_H) / 2;
    g_rcClearTexts = {w - BTN_W - 10, clrY, w - 10, clrY + BTN_H};
    g_rcClearAll   = {g_rcClearTexts.left - BTN_W - 8, clrY, g_rcClearTexts.left - 8, clrY + BTN_H};

    // --- Content area ---
    g_rcContent    = {1, TITLE_H + 1, w - SCROLLBAR_W - 1, h - BOTTOM_H - 1};
    g_rcScrollbar  = {g_rcContent.right, TITLE_H, w, h - BOTTOM_H};

    // Reposition the content container window and scrollbar
    if (g_hContentWnd) {
        SetWindowPos(g_hContentWnd, nullptr,
                     g_rcContent.left, g_rcContent.top,
                     g_rcContent.right - g_rcContent.left,
                     g_rcContent.bottom - g_rcContent.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (g_hScrollbar) {
        SetWindowPos(g_hScrollbar, nullptr,
                     g_rcScrollbar.left, g_rcScrollbar.top,
                     SCROLLBAR_W, g_rcScrollbar.bottom - g_rcScrollbar.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Reposition all item controls
    for (int i = 0; i < (int)g_items.size(); i++) {
        RepositionItem(i);
    }
    SyncScrollbar();
}

// ============================================================================
//  ITEM MANAGEMENT
// ============================================================================

void RepositionItem(int idx) {
    if (idx < 0 || idx >= (int)g_items.size()) return;
    PasteItem& it = g_items[idx];
    int cw = g_rcContent.right - g_rcContent.left;   // content width
    int cy = idx * (ITEM_H + ITEM_GAP) - g_iScrollPos;

    int editH    = 24;
    int editY    = cy + (ITEM_H - editH) / 2;
    int btnW     = 22;
    int btnY     = cy + (ITEM_H - ICON_H) / 2;
    int sendW    = 44;

    // Layout: [Edit (flexible)] [Clear btn] [Send btn]
    int rightMargin = 8;
    int sendX    = cw - sendW - rightMargin;
    int clearX   = sendX - btnW - 4;
    int editX    = 8;
    int editW    = clearX - editX - 4;

    if (editW < 60) editW = 60;

    it.rcEdit  = {editX,  editY, editX + editW,        editY + editH};
    it.rcClear = {clearX, btnY,  clearX + btnW,         btnY + ICON_H};
    it.rcSend  = {sendX,  btnY,  sendX + sendW,         btnY + ICON_H};

    if (it.hEdit) {
        SetWindowPos(it.hEdit, nullptr,
                     it.rcEdit.left, it.rcEdit.top,
                     it.rcEdit.right - it.rcEdit.left,
                     it.rcEdit.bottom - it.rcEdit.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        // Show/hide based on whether visible
        bool vis = (cy + ITEM_H > 0 && cy < (g_rcContent.bottom - g_rcContent.top));
        ShowWindow(it.hEdit, vis ? SW_SHOW : SW_HIDE);
    }
}

// Values passed via subclass ID (stored as DWORD_PTR in SetWindowSubclass)
struct EditCtx {
    std::wstring oldText; // Text on focus entry (for undo tracking)
};

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
                                          UINT_PTR /*subclassId*/, DWORD_PTR /*refData*/) {
    switch (msg) {
    case WM_ERASEBKGND: {
        RECT rc; GetClientRect(hWnd, &rc);
        HDC hdc = (HDC)wp;
        HBRUSH br = CreateSolidBrush(RGB(255, 255, 255)); // white edit bg
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        return 1;
    }
    case WM_SETFOCUS: {
        // Save current text for potential undo
        int len = GetWindowTextLengthW(hWnd);
        std::wstring* pOld = new std::wstring(len, L'\0');
        if (len > 0) GetWindowTextW(hWnd, &(*pOld)[0], len + 1);
        SetPropW(hWnd, L"OLD_TEXT", (HANDLE)pOld);
        break;
    }
    case WM_KILLFOCUS: {
        // Check if text changed and push undo
        std::wstring* pOld = (std::wstring*)GetPropW(hWnd, L"OLD_TEXT");
        if (pOld) {
            int idx = GetDlgCtrlID(hWnd) - CID_ITEM_BASE;
            int newLen = GetWindowTextLengthW(hWnd);
            std::wstring newText(newLen, L'\0');
            if (newLen > 0) GetWindowTextW(hWnd, &newText[0], newLen + 1);
            if (*pOld != newText && idx >= 0) {
                UndoEntry ue;
                ue.type      = UndoType::TextChanged;
                ue.itemIndex = idx;
                ue.oldText   = *pOld;
                ue.newText   = newText;
                PushUndo(ue);
            }
            delete pOld;
            RemovePropW(hWnd, L"OLD_TEXT");
        }
        break;
    }
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_DESTROY: {
        std::wstring* pOld = (std::wstring*)GetPropW(hWnd, L"OLD_TEXT");
        if (pOld) { delete pOld; RemovePropW(hWnd, L"OLD_TEXT"); }
        break;
    }
    }
    return DefSubclassProc(hWnd, msg, wp, lp);
}

void CreateItemControls(int idx) {
    if (idx < 0 || idx >= (int)g_items.size()) return;
    PasteItem& it = g_items[idx];

    if (!it.hEdit) {
        it.hEdit = CreateWindowExW(0, L"EDIT", L"",
                     WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP,
                     0, 0, 100, 24,
                     g_hContentWnd, (HMENU)(UINT_PTR)(CID_ITEM_BASE + idx),
                     g_hInst, nullptr);
        // Dark theme + undo tracking via subclass
        SetWindowSubclass(it.hEdit, EditSubclassProc, (UINT_PTR)idx, 0);
    }

    RepositionItem(idx);

    // Set edit control font
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, L"Segoe UI");
    SendMessageW(it.hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void DestroyItemControls(int idx) {
    if (idx < 0 || idx >= (int)g_items.size()) return;
    PasteItem& it = g_items[idx];
    if (it.hEdit) {
        DestroyWindow(it.hEdit);
        it.hEdit = nullptr;
    }
}

void RebuildItems() {
    // Destroy all existing controls
    for (auto& it : g_items) {
        if (it.hEdit) DestroyWindow(it.hEdit);
    }
    g_items.clear();
}

// ============================================================================
//  SCROLLBAR
// ============================================================================

void SyncScrollbar() {
    if (!g_hScrollbar) return;
    int contentH = g_rcContent.bottom - g_rcContent.top;
    int totalH   = (int)g_items.size() * (ITEM_H + ITEM_GAP) + 8;
    if (totalH < contentH) totalH = contentH;

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = totalH - 1;
    si.nPage  = contentH;
    si.nPos   = g_iScrollPos;
    SetScrollInfo(g_hScrollbar, SB_CTL, &si, TRUE);
}

// ============================================================================
//  CLIPBOARD & PASTE
// ============================================================================

void SetClipText(const std::wstring& text) {
    if (!OpenClipboard(g_hWnd)) return;
    EmptyClipboard();
    size_t sz = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sz);
    if (hMem) {
        wchar_t* p = (wchar_t*)GlobalLock(hMem);
        if (p) {
            memcpy(p, text.c_str(), sz);
            GlobalUnlock(hMem);
        }
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
}

void SimulatePaste() {
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    inputs[1].ki.dwFlags = 0;

    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
}

// ============================================================================
//  UNDO SYSTEM
// ============================================================================

void PushUndo(const UndoEntry& e) {
    g_undoStack.push(e);
    // Limit undo stack size
    if (g_undoStack.size() > 200) {
        std::stack<UndoEntry> tmp;
        while (g_undoStack.size() > 100) g_undoStack.pop();
        // Can't easily trim bottom of stack; just keep 200 max
    }
}

// ============================================================================
//  STATE PERSISTENCE
// ============================================================================

void SaveState() {
    wchar_t path[MAX_PATH];
    GetConfigPath(path, MAX_PATH);

    // Simple text-based config: one line per item text, plus metadata
    std::wstring data;
    data += L"# BatchPasteTool Config\n";
    data += L"pinned=" + std::to_wstring(g_bPinned ? 1 : 0) + L"\n";
    data += L"transparency=" + std::to_wstring(g_iTransparency) + L"\n";
    data += L"items=" + std::to_wstring(g_items.size()) + L"\n";

    // Save window position
    RECT rc; GetWindowRect(g_hWnd, &rc);
    data += L"win_x=" + std::to_wstring(rc.left) + L"\n";
    data += L"win_y=" + std::to_wstring(rc.top) + L"\n";
    data += L"win_w=" + std::to_wstring(rc.right - rc.left) + L"\n";
    data += L"win_h=" + std::to_wstring(rc.bottom - rc.top) + L"\n";

    for (size_t i = 0; i < g_items.size(); i++) {
        wchar_t buf[4096];
        int len = GetWindowTextW(g_items[i].hEdit, buf, 4095);
        buf[len] = 0;
        // Escape newlines with \n literal
        std::wstring escaped;
        for (int j = 0; j < len; j++) {
            if (buf[j] == L'\\') escaped += L"\\\\";
            else if (buf[j] == L'\n') escaped += L"\\n";
            else if (buf[j] == L'\r') escaped += L"\\r";
            else escaped.push_back(buf[j]);
        }
        data += L"text" + std::to_wstring(i) + L"=" + escaped + L"\n";
    }

    // Write file as UTF-8
    DWORD dwWritten;
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Write BOM
        unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
        WriteFile(hFile, bom, 3, &dwWritten, nullptr);
        // Convert to UTF-8
        int utf8len = WideCharToMultiByte(CP_UTF8, 0, data.c_str(), -1, nullptr, 0, nullptr, nullptr);
        char* utf8 = new char[utf8len];
        WideCharToMultiByte(CP_UTF8, 0, data.c_str(), -1, utf8, utf8len, nullptr, nullptr);
        WriteFile(hFile, utf8, utf8len - 1, &dwWritten, nullptr);
        delete[] utf8;
        CloseHandle(hFile);
    }
}

std::wstring UnescapeText(const std::wstring& s) {
    std::wstring result;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == L'\\' && i + 1 < s.length()) {
            wchar_t next = s[i + 1];
            if (next == L'n')  { result.push_back(L'\n'); i++; }
            else if (next == L'r') { result.push_back(L'\r'); i++; }
            else if (next == L'\\') { result.push_back(L'\\'); i++; }
            else result.push_back(s[i]);
        } else {
            result.push_back(s[i]);
        }
    }
    return result;
}

void LoadState() {
    wchar_t path[MAX_PATH];
    GetConfigPath(path, MAX_PATH);

    // Check if config exists
    DWORD attr = GetFileAttributesW(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        // No config — start with one empty item
        OnAddItem();
        return;
    }

    // Read whole file
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) { OnAddItem(); return; }

    DWORD size = GetFileSize(hFile, nullptr);
    if (size == 0 || size > 1024 * 1024) { CloseHandle(hFile); OnAddItem(); return; }

    char* raw = new char[size + 1];
    DWORD read;
    ReadFile(hFile, raw, size, &read, nullptr);
    raw[read] = 0;
    CloseHandle(hFile);

    // Convert UTF-8 to wchar
    int wlen = MultiByteToWideChar(CP_UTF8, 0, raw, -1, nullptr, 0);
    wchar_t* wbuf = new wchar_t[wlen + 1];
    MultiByteToWideChar(CP_UTF8, 0, raw, -1, wbuf, wlen);
    wbuf[wlen] = 0;
    delete[] raw;

    std::wstring content(wbuf);
    delete[] wbuf;

    // Parse line by line
    int itemCount = 1;
    int winX = CW_USEDEFAULT, winY = CW_USEDEFAULT, winW = 600, winH = 480;
    std::vector<std::wstring> itemTexts;

    size_t pos = 0;
    while (pos < content.length()) {
        size_t nl = content.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = content.length();
        std::wstring line = content.substr(pos, nl - pos);
        pos = nl + 1;
        // Trim \r
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (line.empty() || line[0] == L'#') continue;

        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        std::wstring key = line.substr(0, eq);
        std::wstring val = line.substr(eq + 1);

        if (key == L"pinned")       g_bPinned = (val == L"1");
        else if (key == L"transparency") g_iTransparency = _wtoi(val.c_str());
        else if (key == L"items")   itemCount = _wtoi(val.c_str());
        else if (key == L"win_x")   winX = _wtoi(val.c_str());
        else if (key == L"win_y")   winY = _wtoi(val.c_str());
        else if (key == L"win_w")   winW = _wtoi(val.c_str());
        else if (key == L"win_h")   winH = _wtoi(val.c_str());
        else if (key.find(L"text") == 0) {
            int idx = _wtoi(key.substr(4).c_str());
            if (idx >= (int)itemTexts.size()) itemTexts.resize(idx + 1);
            itemTexts[idx] = UnescapeText(val);
        }
    }

    // Clamp values
    if (g_iTransparency < 51)  g_iTransparency = 51;
    if (g_iTransparency > 255) g_iTransparency = 255;
    if (itemCount < 1) itemCount = 1;
    if (itemCount > 100) itemCount = 100;
    if (winW < MIN_WIN_W) winW = MIN_WIN_W;
    if (winH < MIN_WIN_H) winH = MIN_WIN_H;

    // Set window position
    SetWindowPos(g_hWnd, nullptr, winX, winY, winW, winH, SWP_NOZORDER);

    // Create items
    while ((int)itemTexts.size() < itemCount) itemTexts.push_back(L"");
    for (int i = 0; i < itemCount; i++) {
        OnAddItem();
        if (i < (int)itemTexts.size() && g_items[i].hEdit) {
            SetWindowTextW(g_items[i].hEdit, itemTexts[i].c_str());
        }
    }

    ApplyPin();
    ApplyTransparency();
}

// ============================================================================
//  ACTIONS
// ============================================================================

void OnAddItem() {
    PasteItem it = {};
    g_items.push_back(it);
    int idx = (int)g_items.size() - 1;
    CreateItemControls(idx);

    // Record undo
    UndoEntry ue;
    ue.type = UndoType::ItemAdded;
    ue.itemIndex = idx;
    PushUndo(ue);

    SyncScrollbar();
    InvalidateRect(g_hWnd, nullptr, TRUE);
}

void OnDeleteItem() {
    if (g_items.size() <= 1) return; // Keep at least one
    int idx = (int)g_items.size() - 1;

    // Save text for undo
    UndoEntry ue;
    ue.type = UndoType::ItemDeleted;
    ue.itemIndex = idx;
    wchar_t buf[4096];
    GetWindowTextW(g_items[idx].hEdit, buf, 4095);
    ue.oldItemTexts.push_back(buf);
    PushUndo(ue);

    DestroyItemControls(idx);
    g_items.pop_back();

    SyncScrollbar();
    InvalidateRect(g_hWnd, nullptr, TRUE);
}

void OnUndo() {
    if (g_undoStack.empty()) return;
    UndoEntry e = g_undoStack.top();
    g_undoStack.pop();

    switch (e.type) {
    case UndoType::TextChanged:
        if (e.itemIndex >= 0 && e.itemIndex < (int)g_items.size()) {
            SetWindowTextW(g_items[e.itemIndex].hEdit, e.oldText.c_str());
        }
        break;

    case UndoType::ItemAdded:
        // Undo add = delete last item
        if (!g_items.empty() && g_items.size() > 1) {
            int last = (int)g_items.size() - 1;
            DestroyItemControls(last);
            g_items.pop_back();
        }
        break;

    case UndoType::ItemDeleted:
        // Undo delete = re-add item
        {
            PasteItem it = {};
            g_items.push_back(it);
            int ni = (int)g_items.size() - 1;
            CreateItemControls(ni);
            if (!e.oldItemTexts.empty()) {
                SetWindowTextW(g_items[ni].hEdit, e.oldItemTexts[0].c_str());
            }
        }
        break;

    case UndoType::AllItemsCleared:
        // Recreate items from saved texts
        for (auto& it : g_items) { if (it.hEdit) DestroyWindow(it.hEdit); }
        g_items.clear();
        for (size_t i = 0; i < e.oldItemTexts.size(); i++) {
            PasteItem it = {};
            g_items.push_back(it);
            CreateItemControls((int)i);
            SetWindowTextW(g_items[i].hEdit, e.oldItemTexts[i].c_str());
        }
        break;

    case UndoType::AllTextsCleared:
        // Restore texts
        for (size_t i = 0; i < e.oldItemTexts.size() && i < g_items.size(); i++) {
            SetWindowTextW(g_items[i].hEdit, e.oldItemTexts[i].c_str());
        }
        break;
    }

    SyncScrollbar();
    InvalidateRect(g_hWnd, nullptr, TRUE);
}

void OnClearAllItems() {
    if (g_items.size() <= 1) {
        // Just clear the text of the last item
        SetWindowTextW(g_items[0].hEdit, L"");
        return;
    }

    // Save state for undo
    UndoEntry ue;
    ue.type = UndoType::AllItemsCleared;
    for (auto& it : g_items) {
        wchar_t buf[4096];
        GetWindowTextW(it.hEdit, buf, 4095);
        ue.oldItemTexts.push_back(buf);
    }
    PushUndo(ue);

    // Remove all items except first
    while (g_items.size() > 1) {
        int last = (int)g_items.size() - 1;
        DestroyItemControls(last);
        g_items.pop_back();
    }
    // Clear the first item's text
    if (!g_items.empty() && g_items[0].hEdit) {
        SetWindowTextW(g_items[0].hEdit, L"");
    }

    g_iScrollPos = 0;
    SyncScrollbar();
    InvalidateRect(g_hWnd, nullptr, TRUE);
}

void OnClearAllTexts() {
    // Save for undo
    UndoEntry ue;
    ue.type = UndoType::AllTextsCleared;
    for (auto& it : g_items) {
        wchar_t buf[4096];
        GetWindowTextW(it.hEdit, buf, 4095);
        ue.oldItemTexts.push_back(buf);
    }
    PushUndo(ue);

    for (auto& it : g_items) {
        if (it.hEdit) SetWindowTextW(it.hEdit, L"");
    }
}

void OnSend(int idx) {
    if (idx < 0 || idx >= (int)g_items.size()) return;
    PasteItem& it = g_items[idx];
    if (!it.hEdit) return;

    int len = GetWindowTextLengthW(it.hEdit);
    if (len == 0) return;
    wchar_t* buf = new wchar_t[len + 1];
    GetWindowTextW(it.hEdit, buf, len + 1);
    std::wstring text(buf);
    delete[] buf;

    // Copy to clipboard
    SetClipText(text);

    // Switch to last known target window and paste
    if (g_hTargetWnd && IsWindow(g_hTargetWnd) && g_hTargetWnd != g_hWnd) {
        SetForegroundWindow(g_hTargetWnd);
        Sleep(40);
    }

    SimulatePaste();
}

void OnClearItem(int idx) {
    if (idx < 0 || idx >= (int)g_items.size()) return;
    PasteItem& it = g_items[idx];
    if (!it.hEdit) return;

    // Save for undo
    wchar_t buf[4096];
    GetWindowTextW(it.hEdit, buf, 4095);
    if (wcslen(buf) == 0) return; // Nothing to clear

    UndoEntry ue;
    ue.type = UndoType::TextChanged;
    ue.itemIndex = idx;
    ue.oldText = buf;
    ue.newText = L"";
    PushUndo(ue);

    SetWindowTextW(it.hEdit, L"");
}

// ============================================================================
//  APPLY SETTINGS
// ============================================================================

void ApplyTransparency() {
    LONG_PTR exStyle = GetWindowLongPtrW(g_hWnd, GWL_EXSTYLE);
    if (g_iTransparency >= 255) {
        // Fully opaque — remove layered style
        SetWindowLongPtrW(g_hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        RedrawWindow(g_hWnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    } else {
        SetWindowLongPtrW(g_hWnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        SetLayeredWindowAttributes(g_hWnd, 0, (BYTE)g_iTransparency, LWA_ALPHA);
    }
}

void ApplyPin() {
    SetWindowPos(g_hWnd,
                 g_bPinned ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// ============================================================================
//  DRAWING
// ============================================================================

void DrawTitleBar(Gdiplus::Graphics& g) {
    // Background
    g.FillRectangle(g_pBrTitleBg, ToGdipRect(g_rcTitleBar));

    // App name — darker and slightly larger
    Gdiplus::Font titleFont(L"Segoe UI", 11.0f, Gdiplus::FontStyleBold);
    Gdiplus::SolidBrush textBr(Gdiplus::Color(32, 32, 32));
    g.DrawString(APP_NAME, -1, &titleFont,
                 Gdiplus::RectF(Gdiplus::REAL(g_rcName.left), Gdiplus::REAL(g_rcName.top),
                                Gdiplus::REAL(g_rcName.right - g_rcName.left),
                                Gdiplus::REAL(g_rcName.bottom - g_rcName.top)),
                 nullptr, &textBr);

    // Pin button
    if (g_pBmpPin) {
        g.DrawImage(g_pBmpPin, Gdiplus::Rect(g_rcPinBtn.left, g_rcPinBtn.top, ICON_W, ICON_H));
        if (g_bPinned) {
            Gdiplus::Pen pinPen(Gdiplus::Color(66, 133, 244), 3.0f);
            g.DrawRectangle(&pinPen, Gdiplus::Rect(g_rcPinBtn.left + 1, g_rcPinBtn.top + 1, ICON_W - 3, ICON_H - 3));
        }
        if (g_bPinHover) {
            Gdiplus::SolidBrush hov(Gdiplus::Color(120, 66, 133, 244));
            g.FillRectangle(&hov, Gdiplus::Rect(g_rcPinBtn.left, g_rcPinBtn.top, ICON_W, ICON_H));
        }
    }

    // Transparency icon
    if (g_pBmpTransp) {
        g.DrawImage(g_pBmpTransp, Gdiplus::Rect(g_rcTranspIcon.left, g_rcTranspIcon.top, ICON_W, ICON_H));
        if (g_bTranspHover) {
            Gdiplus::SolidBrush hov(Gdiplus::Color(120, 66, 133, 244));
            g.FillRectangle(&hov, Gdiplus::Rect(g_rcTranspIcon.left, g_rcTranspIcon.top, ICON_W, ICON_H));
        }
    }

    // Window control buttons — darker + thicker lines
    Gdiplus::Pen btnPen(Gdiplus::Color(50, 50, 50), 2.0f);
    Gdiplus::Pen closePen(Gdiplus::Color(50, 50, 50), 2.2f);

    // Min button
    {
        int cx = (INT)(g_rcMinBtn.left + (WINBTN_W - 12) / 2);
        int cy = (INT)(g_rcMinBtn.top + (WINBTN_H - 4) / 2 + 1);
        if (g_bMinHover) {
            g.FillRectangle(g_pBrBtnHover, Gdiplus::Rect(g_rcMinBtn.left, g_rcMinBtn.top, WINBTN_W, WINBTN_H));
            Gdiplus::Pen hovMinPen(Gdiplus::Color(50, 50, 50), 2.0f);
            g.DrawLine(&hovMinPen, cx, cy + 5, cx + 12, cy + 5);
        } else {
            g.DrawLine(&btnPen, cx, cy + 5, cx + 12, cy + 5);
        }
    }

    // Max button
    {
        int cx = (INT)(g_rcMaxBtn.left + (WINBTN_W - 10) / 2);
        int cy = (INT)(g_rcMaxBtn.top + (WINBTN_H - 10) / 2);
        if (g_bMaxHover) {
            g.FillRectangle(g_pBrBtnHover, Gdiplus::Rect(g_rcMaxBtn.left, g_rcMaxBtn.top, WINBTN_W, WINBTN_H));
            Gdiplus::Pen hovMaxPen(Gdiplus::Color(50, 50, 50), 2.0f);
            g.DrawRectangle(&hovMaxPen, Gdiplus::Rect(cx, cy, 10, 10));
        } else {
            g.DrawRectangle(&btnPen, Gdiplus::Rect(cx, cy, 10, 10));
        }
    }

    // Close button
    {
        if (g_bCloseHover) {
            g.FillRectangle(g_pBrCloseHov, Gdiplus::Rect(g_rcCloseBtn.left, g_rcCloseBtn.top, WINBTN_W, WINBTN_H));
            Gdiplus::Pen whitePen(Gdiplus::Color(255, 255, 255), 2.5f);
            int cx = (INT)(g_rcCloseBtn.left + (WINBTN_W - 10) / 2);
            int cy = (INT)(g_rcCloseBtn.top + (WINBTN_H - 10) / 2);
            g.DrawLine(&whitePen, cx, cy, cx + 10, cy + 10);
            g.DrawLine(&whitePen, cx + 10, cy, cx, cy + 10);
        } else {
            int cx = (INT)(g_rcCloseBtn.left + (WINBTN_W - 10) / 2);
            int cy = (INT)(g_rcCloseBtn.top + (WINBTN_H - 10) / 2);
            g.DrawLine(&closePen, cx, cy, cx + 10, cy + 10);
            g.DrawLine(&closePen, cx + 10, cy, cx, cy + 10);
        }
    }
}

void DrawSlider(Gdiplus::Graphics& g) {
    RECT& rc = g_rcTranspSlider;
    int w = (INT)(rc.right - rc.left);
    int h = (INT)(rc.bottom - rc.top);
    int radius = h / 2 - 1;

    // Track background — darker for contrast
    Gdiplus::SolidBrush trackBg(Gdiplus::Color(180, 180, 180));
    g.FillRectangle(&trackBg, (INT)rc.left, (INT)(rc.top + h / 2 - 3), w, 6);

    // Thumb position
    float ratio = (float)(g_iTransparency - 51) / (255.0f - 51.0f);
    int thumbX = (INT)(rc.left + 2 + ratio * (w - 14));

    // Filled portion (left of thumb) - blue
    g.FillRectangle(g_pBrSliderFill, (INT)(rc.left + 2), (INT)(rc.top + h / 2 - 3),
                    (INT)(thumbX - rc.left - 2), 6);

    // Empty portion (right of thumb) — slightly more visible
    Gdiplus::SolidBrush emptyBr(Gdiplus::Color(200, 200, 200));
    g.FillRectangle(&emptyBr, (INT)(thumbX + 12), (INT)(rc.top + h / 2 - 3),
                    (INT)(rc.right - thumbX - 14), 6);

    // Thumb (circle) — darker border
    int thumbCY = (INT)(rc.top + h / 2);
    g.FillEllipse(g_pBrSliderThumb, thumbX, thumbCY - (radius - 1), 12, 12);
    Gdiplus::Pen thumbBorder(Gdiplus::Color(40, 100, 200), 1.5f);
    g.DrawEllipse(&thumbBorder, thumbX, thumbCY - (radius - 1), 12, 12);

    // Hover glow
    if (g_bSliderHover || g_bDragging) {
        Gdiplus::Pen glowPen(Gdiplus::Color(160, 66, 133, 244), 2.0f);
        g.DrawEllipse(&glowPen, thumbX - 1, thumbCY - radius, 14, 14);
    }
}

void DrawContentArea(Gdiplus::Graphics& g) {
    RECT& rc = g_rcContent;
    int cw = (INT)(rc.right - rc.left);
    int ch = (INT)(rc.bottom - rc.top);
    int offX = (INT)rc.left;
    int offY = (INT)rc.top;

    // Background
    g.FillRectangle(g_pBrContentBg, ToGdipRect(rc));

    // Draw each visible item
    for (int i = 0; i < (int)g_items.size(); i++) {
        PasteItem& it = g_items[i];
        int cy = i * (ITEM_H + ITEM_GAP) - g_iScrollPos;
        if (cy + ITEM_H < 0 || cy > ch) continue;

        // Item background — slightly darker for contrast against white
        Gdiplus::SolidBrush itemBg(Gdiplus::Color(240, 240, 240));
        g.FillRectangle(&itemBg, offX + 4, offY + cy + 2, cw - 8, ITEM_H - 4);

        // Separator line — darker
        Gdiplus::Pen sepPen(Gdiplus::Color(190, 190, 190), 1.0f);
        int sepY = offY + cy + ITEM_H + 1;
        g.DrawLine(&sepPen, offX + 4, sepY, offX + cw - 4, sepY);

        // Send button
        RECT sr = it.rcSend;
        int btnLeft   = offX + (INT)sr.left;
        int btnTop    = offY + (INT)sr.top    - g_iScrollPos;
        int btnWidth  = (INT)(sr.right - sr.left);
        int btnHeight = (INT)(sr.bottom - sr.top);
        bool sendHover = (g_iHoverItem == i);
        {
            Gdiplus::Color btnCol(sendHover ? Gdiplus::Color(20, 80, 180) : Gdiplus::Color(50, 110, 220));
            Gdiplus::SolidBrush btnBg(btnCol);
            Gdiplus::Rect btnR(btnLeft, btnTop, btnWidth, btnHeight);
            g.FillRectangle(&btnBg, btnR);
            Gdiplus::Font f(L"Segoe UI", 9.5f, Gdiplus::FontStyleBold);
            Gdiplus::StringFormat fmt;
            fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
            fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::SolidBrush sendText(Gdiplus::Color(255, 255, 255));
            Gdiplus::RectF srf((Gdiplus::REAL)btnLeft, (Gdiplus::REAL)btnTop,
                               (Gdiplus::REAL)btnWidth, (Gdiplus::REAL)btnHeight);
            g.DrawString(L"Send", -1, &f, srf, &fmt, &sendText);
        }

        // Clear button — darker X
        RECT cr = it.rcClear;
        int clrLeft   = offX + (INT)cr.left;
        int clrTop    = offY + (INT)cr.top    - g_iScrollPos;
        int clrWidth  = (INT)(cr.right - cr.left);
        int clrHeight = (INT)(cr.bottom - cr.top);
        bool clearHover = (g_iHoverClear == i);
        {
            int ccx = clrLeft + clrWidth / 2;
            int ccy = clrTop  + clrHeight / 2;
            Gdiplus::Color clCol(clearHover ? 220 : 130, clearHover ? 60 : 130, clearHover ? 50 : 130);
            Gdiplus::Pen clPen(clCol, 2.2f);
            int sz = 5;
            g.DrawLine(&clPen, ccx - sz, ccy - sz, ccx + sz, ccy + sz);
            g.DrawLine(&clPen, ccx + sz, ccy - sz, ccx - sz, ccy + sz);
        }
    }

    // "No items" message
    if (g_items.empty()) {
        Gdiplus::Font f(L"Segoe UI", 10.0f);
        Gdiplus::SolidBrush dim(Gdiplus::Color(140, 140, 140));
        Gdiplus::StringFormat fmt;
        fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(L"No items. Click + to add one.", -1, &f,
                     Gdiplus::RectF(Gdiplus::REAL(rc.left), Gdiplus::REAL(rc.top),
                                    Gdiplus::REAL(cw), Gdiplus::REAL(ch)),
                     &fmt, &dim);
    }
}

void DrawBottomBar(Gdiplus::Graphics& g) {
    RECT& rc = g_rcBottom;
    g.FillRectangle(g_pBrTitleBg, ToGdipRect(rc));

    // Separator line — darker
    Gdiplus::Pen sepPen(Gdiplus::Color(180, 180, 180), 1.0f);
    g.DrawLine(&sepPen, (INT)rc.left, (INT)rc.top, (INT)rc.right, (INT)rc.top);

    // Left icon buttons
    auto drawIconBtn = [&](RECT& r, Gdiplus::Bitmap* bmp, bool hover, bool dimmed) {
        if (bmp) {
            Gdiplus::Rect dest((INT)r.left, (INT)r.top, ICON_W, ICON_H);
            if (dimmed) {
                Gdiplus::ColorMatrix cm = {};
                cm.m[0][0] = 1; cm.m[1][1] = 1; cm.m[2][2] = 1; cm.m[3][3] = 0.25f; cm.m[4][4] = 1;
                Gdiplus::ImageAttributes ia;
                ia.SetColorMatrix(&cm);
                g.DrawImage(bmp, dest, 0, 0, (INT)bmp->GetWidth(), (INT)bmp->GetHeight(),
                           Gdiplus::UnitPixel, &ia);
            } else {
                g.DrawImage(bmp, dest);
            }
        }
        if (hover) {
            Gdiplus::SolidBrush hov(Gdiplus::Color(130, 66, 133, 244));
            g.FillRectangle(&hov, Gdiplus::Rect((INT)r.left, (INT)r.top,
                            (INT)(r.right - r.left), (INT)(r.bottom - r.top)));
        }
    };

    drawIconBtn(g_rcAddBtn,  g_pBmpAdd,    g_bAddHover,    false);
    drawIconBtn(g_rcDelBtn,  g_pBmpDelete, g_bDelHover,    g_items.size() <= 1);
    drawIconBtn(g_rcUndoBtn, g_pBmpUndo,   g_bUndoHover,   g_undoStack.empty());

    // Right text buttons — larger bolder font
    auto drawTextBtn = [&](RECT& r, const wchar_t* label, bool hover) {
        Gdiplus::Color btnCol(hover ? Gdiplus::Color(20, 80, 180) : Gdiplus::Color(50, 110, 220));
        Gdiplus::SolidBrush btnBg(btnCol);
        Gdiplus::Rect btnR((INT)r.left, (INT)r.top, (INT)(r.right - r.left), (INT)(r.bottom - r.top));
        g.FillRectangle(&btnBg, btnR);
        Gdiplus::Font f(L"Segoe UI", 9.5f, Gdiplus::FontStyleBold);
        Gdiplus::StringFormat fmt;
        fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::SolidBrush whiteBr(Gdiplus::Color(255, 255, 255));
        g.DrawString(label, -1, &f,
                     Gdiplus::RectF(Gdiplus::REAL(r.left), Gdiplus::REAL(r.top),
                                    Gdiplus::REAL(r.right - r.left), Gdiplus::REAL(r.bottom - r.top)),
                     &fmt, &whiteBr);
    };

    drawTextBtn(g_rcClearAll,   L"Clear All",   g_bClrAllHover);
    drawTextBtn(g_rcClearTexts, L"Clear Texts",  g_bClrTxtHover);
}

void OnPaint(HWND hwnd, HDC hdc) {
    RECT rc; GetClientRect(hwnd, &rc);
    int w = (INT)(rc.right - rc.left);
    int h = (INT)(rc.bottom - rc.top);

    // Double-buffer
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // Fill with light background
    HBRUSH bgBr = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(memDC, &rc, bgBr);
    DeleteObject(bgBr);

    Gdiplus::Graphics g(memDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    DrawTitleBar(g);
    DrawContentArea(g);
    DrawSlider(g);
    DrawBottomBar(g);

    // Blit
    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

// ============================================================================
//  HOVER TRACKING
// ============================================================================

void UpdateAllHovers(int x, int y) {
    g_bCloseHover   = PtInRc(g_rcCloseBtn,   x, y);
    g_bMinHover     = PtInRc(g_rcMinBtn,     x, y);
    g_bMaxHover     = PtInRc(g_rcMaxBtn,     x, y);
    g_bPinHover     = PtInRc(g_rcPinBtn,     x, y);
    g_bTranspHover  = PtInRc(g_rcTranspIcon, x, y);
    g_bSliderHover  = PtInRc(g_rcTranspSlider, x, y);
    g_bAddHover     = PtInRc(g_rcAddBtn,     x, y);
    g_bDelHover     = PtInRc(g_rcDelBtn,     x, y);
    g_bUndoHover    = PtInRc(g_rcUndoBtn,    x, y);
    g_bClrAllHover  = PtInRc(g_rcClearAll,   x, y);
    g_bClrTxtHover  = PtInRc(g_rcClearTexts, x, y);

    // Per-item hovers — convert content-relative rects to main-window coords
    g_iHoverItem  = -1;
    g_iHoverClear = -1;
    int offX = (INT)g_rcContent.left;
    int offY = (INT)g_rcContent.top;
    for (int i = 0; i < (int)g_items.size(); i++) {
        RECT sr = g_items[i].rcSend;
        RECT cr = g_items[i].rcClear;
        OffsetRect(&sr, offX, offY);
        OffsetRect(&cr, offX, offY);
        sr.top    -= g_iScrollPos;  sr.bottom -= g_iScrollPos;
        cr.top    -= g_iScrollPos;  cr.bottom -= g_iScrollPos;
        if (PtInRc(sr, x, y)) g_iHoverItem = i;
        if (PtInRc(cr, x, y)) g_iHoverClear = i;
    }
}

LRESULT HandleMouseMove(HWND hwnd, int x, int y) {
    bool wasHover = g_bCloseHover || g_bMinHover || g_bMaxHover ||
                    g_bPinHover || g_bTranspHover || g_bSliderHover ||
                    g_bAddHover || g_bDelHover || g_bUndoHover ||
                    g_bClrAllHover || g_bClrTxtHover ||
                    (g_iHoverItem >= 0) || (g_iHoverClear >= 0);

    UpdateAllHovers(x, y);

    bool isHover = g_bCloseHover || g_bMinHover || g_bMaxHover ||
                   g_bPinHover || g_bTranspHover || g_bSliderHover ||
                   g_bAddHover || g_bDelHover || g_bUndoHover ||
                   g_bClrAllHover || g_bClrTxtHover ||
                   (g_iHoverItem >= 0) || (g_iHoverClear >= 0);

    if (wasHover != isHover || g_bSliderHover || g_bDragging) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    // Slider dragging
    if (g_bDragging) {
        RECT& sr = g_rcTranspSlider;
        int relX = x - sr.left - 6; // 6 = half thumb width offset
        int range = (sr.right - sr.left) - 14;
        if (range < 1) range = 1;
        float ratio = (float)relX / range;
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        g_iTransparency = 51 + (int)(ratio * (255.0f - 51.0f));
        ApplyTransparency();
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    // Update cursor
    if (g_bSliderHover || g_bDragging) {
        SetCursor(LoadCursorW(nullptr, IDC_HAND));
    } else if (isHover) {
        SetCursor(LoadCursorW(nullptr, IDC_HAND));
    }

    return 0;
}

// ============================================================================
//  WINDOW PROCEDURE
// ============================================================================

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    // ---- Window creation / destruction ----
    case WM_CREATE:
    {
        g_hWnd = hwnd;
        LoadAllPngs();
        InitGdiPlusCache();

        // Create content container (static child window for clipping)
        g_hContentWnd = CreateWindowW(L"STATIC", nullptr,
                          WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                          0, 0, 100, 100, hwnd, nullptr, g_hInst, nullptr);

        // Create scrollbar
        g_hScrollbar = CreateWindowW(L"SCROLLBAR", nullptr,
                         WS_CHILD | WS_VISIBLE | SBS_VERT,
                         0, 0, SCROLLBAR_W, 100, hwnd, nullptr, g_hInst, nullptr);

        // Timer for foreground window tracking
        SetTimer(hwnd, IDT_CHECK_FG, 300, nullptr);
        // Timer for auto-save
        SetTimer(hwnd, IDT_AUTOSAVE, 3000, nullptr);

        LoadState();

        // Clear undo stack that got populated during initial load
        while (!g_undoStack.empty()) g_undoStack.pop();

        // Set window icon from PNG
        if (g_pBmpAppIcon) {
            HBITMAP hBmp;
            if (g_pBmpAppIcon->GetHBITMAP(Gdiplus::Color::Transparent, &hBmp) == Gdiplus::Ok) {
                HICON hIcon = nullptr;
                ICONINFO ii = {};
                ii.fIcon = TRUE;
                ii.hbmColor = hBmp;
                // Create monochrome mask
                BITMAP bm;
                GetObjectW(hBmp, sizeof(bm), &bm);
                ii.hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, nullptr);
                hIcon = CreateIconIndirect(&ii);
                if (hIcon) {
                    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                }
                DeleteObject(ii.hbmMask);
            }
        }

        UpdateLayout();
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, IDT_CHECK_FG);
        KillTimer(hwnd, IDT_AUTOSAVE);
        SaveState();
        RebuildItems();
        FreeAllPngs();
        FreeGdiPlusCache();
        PostQuitMessage(0);
        return 0;

    // ---- Timers ----
    case WM_TIMER:
        if (wp == IDT_CHECK_FG) {
            HWND hFg = GetForegroundWindow();
            if (hFg && hFg != g_hWnd) {
                g_hTargetWnd = hFg;
            }
        } else if (wp == IDT_AUTOSAVE) {
            SaveState();
        }
        return 0;

    // ---- Sizing ----
    case WM_SIZE:
        UpdateLayout();
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lp;
        mmi->ptMinTrackSize.x = MIN_WIN_W;
        mmi->ptMinTrackSize.y = MIN_WIN_H;
        return 0;
    }

    // ---- Hit-test for border resize and title-bar drag ----
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProcW(hwnd, msg, wp, lp);
        // Convert screen coords to client
        POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);

        // If in title bar area, allow dragging
        if (pt.y >= 0 && pt.y < TITLE_H) {
            // Don't allow drag on interactive elements
            if (PtInRc(g_rcCloseBtn, pt.x, pt.y) ||
                PtInRc(g_rcMinBtn,   pt.x, pt.y) ||
                PtInRc(g_rcMaxBtn,   pt.x, pt.y) ||
                PtInRc(g_rcPinBtn,   pt.x, pt.y) ||
                PtInRc(g_rcTranspIcon, pt.x, pt.y) ||
                PtInRc(g_rcTranspSlider, pt.x, pt.y)) {
                return hit; // Let button clicks pass through
            }
            return HTCAPTION; // Allow window drag from title bar
        }
        return hit;
    }

    // ---- Mouse input (for custom-drawn controls) ----
    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lp);
        int y = GET_Y_LPARAM(lp);

        // Track text change for undo before focus changes
        // (handled via EN_CHANGE in WM_COMMAND)

        // Check title bar buttons
        if (PtInRc(g_rcCloseBtn, x, y)) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (PtInRc(g_rcMinBtn, x, y)) {
            ShowWindow(hwnd, SW_MINIMIZE);
            return 0;
        }
        if (PtInRc(g_rcMaxBtn, x, y)) {
            WINDOWPLACEMENT wp = {sizeof(wp)};
            GetWindowPlacement(hwnd, &wp);
            if (wp.showCmd == SW_MAXIMIZE) {
                ShowWindow(hwnd, SW_RESTORE);
            } else {
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
            return 0;
        }
        if (PtInRc(g_rcPinBtn, x, y)) {
            g_bPinned = !g_bPinned;
            ApplyPin();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRc(g_rcTranspIcon, x, y)) {
            // Toggle between opaque and semi-transparent
            if (g_iTransparency >= 255) {
                g_iTransparency = 180; // ~30% transparent
            } else {
                g_iTransparency = 255;
            }
            ApplyTransparency();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRc(g_rcTranspSlider, x, y)) {
            g_bDragging = true;
            SetCapture(hwnd);

            // Position thumb
            RECT& sr = g_rcTranspSlider;
            int relX = x - sr.left - 6;
            int range = (sr.right - sr.left) - 14;
            if (range < 1) range = 1;
            float ratio = (float)relX / range;
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            g_iTransparency = 51 + (int)(ratio * (255.0f - 51.0f));
            ApplyTransparency();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        // Bottom bar buttons
        if (PtInRc(g_rcAddBtn, x, y))     { OnAddItem();       return 0; }
        if (PtInRc(g_rcDelBtn, x, y))     { OnDeleteItem();    return 0; }
        if (PtInRc(g_rcUndoBtn, x, y))    { OnUndo();          return 0; }
        if (PtInRc(g_rcClearAll, x, y))   { OnClearAllItems(); return 0; }
        if (PtInRc(g_rcClearTexts, x, y)) { OnClearAllTexts(); return 0; }

        // Item Send/Clear buttons — convert content-relative rects to main-window coords
        int offX = (INT)g_rcContent.left;
        int offY = (INT)g_rcContent.top;
        for (int i = 0; i < (int)g_items.size(); i++) {
            RECT sr = g_items[i].rcSend;
            RECT cr = g_items[i].rcClear;
            OffsetRect(&sr, offX, offY);
            OffsetRect(&cr, offX, offY);
            sr.top -= g_iScrollPos; sr.bottom -= g_iScrollPos;
            cr.top -= g_iScrollPos; cr.bottom -= g_iScrollPos;
            if (PtInRc(sr, x, y)) {
                OnSend(i);
                return 0;
            }
            if (PtInRc(cr, x, y)) {
                OnClearItem(i);
                return 0;
            }
        }

        // Not on any custom control — let DefWindowProc handle
        break;
    }

    case WM_LBUTTONUP:
        if (g_bDragging) {
            g_bDragging = false;
            ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE:
        HandleMouseMove(hwnd, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
        g_iScrollPos -= delta * 20;
        if (g_iScrollPos < 0) g_iScrollPos = 0;
        int maxScroll = (int)g_items.size() * (ITEM_H + ITEM_GAP) -
                        (g_rcContent.bottom - g_rcContent.top);
        if (maxScroll < 0) maxScroll = 0;
        if (g_iScrollPos > maxScroll) g_iScrollPos = maxScroll;
        SyncScrollbar();
        for (int i = 0; i < (int)g_items.size(); i++) RepositionItem(i);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    case WM_VSCROLL:
    {
        SCROLLINFO si = {sizeof(si)};
        si.fMask = SIF_ALL;
        GetScrollInfo(g_hScrollbar, SB_CTL, &si);

        switch (LOWORD(wp)) {
        case SB_LINEUP:        si.nPos -= ITEM_H;       break;
        case SB_LINEDOWN:      si.nPos += ITEM_H;       break;
        case SB_PAGEUP:        si.nPos -= si.nPage;     break;
        case SB_PAGEDOWN:      si.nPos += si.nPage;     break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: si.nPos = si.nTrackPos;  break;
        default: return 0;
        }

        if (si.nPos < 0) si.nPos = 0;
        if (si.nPos > si.nMax - (int)si.nPage + 1)
            si.nPos = si.nMax - (int)si.nPage + 1;

        si.fMask = SIF_POS;
        SetScrollInfo(g_hScrollbar, SB_CTL, &si, TRUE);
        g_iScrollPos = si.nPos;

        for (int i = 0; i < (int)g_items.size(); i++) RepositionItem(i);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    // ---- Painting ----
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        OnPaint(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // We draw everything ourselves

    // ---- Commands (edit control notifications) ----
    case WM_COMMAND:
    {
        WORD code = HIWORD(wp);
        WORD cid  = LOWORD(wp);

        if (code == EN_CHANGE) {
            int idx = cid - CID_ITEM_BASE;
            if (idx >= 0 && idx < (int)g_items.size()) {
                // Push undo on text change (debounced — we push on focus loss instead)
                // We'll use a simple flag-based approach
            }
        }
        return 0;
    }

    // ---- Keyboard shortcuts ----
    case WM_KEYDOWN:
    {
        if (wp == 'Z' && GetKeyState(VK_CONTROL) < 0) {
            OnUndo();
            return 0;
        }
        if (wp == 'N' && GetKeyState(VK_CONTROL) < 0) {
            OnAddItem();
            return 0;
        }
        if (wp == 'S' && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) < 0) {
            // Ctrl+Shift+S: send all
            for (int i = 0; i < (int)g_items.size(); i++) {
                OnSend(i);
            }
            return 0;
        }
        break;
    }

    // ---- Window activation tracking ----
    case WM_ACTIVATE:
        // When we lose activation, save any pending text changes
        if (LOWORD(wp) == WA_INACTIVE) {
            // Could push undo here for text changes
        }
        return 0;

    } // end switch

    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================================
//  ENTRY POINT
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    g_hInst = hInstance;

    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiSI;
    ULONG_PTR gdiToken;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, nullptr);

    // Initialize common controls
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = APP_CLASS;

    // Try to load icon from resources; if not available, use default
    wc.hIcon = LoadIconW(hInstance, L"APP_ICON");
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    RegisterClassExW(&wc);

    // Create window (borderless but resizable, with custom title bar)
    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_CONTROLPARENT,
        APP_CLASS, APP_NAME,
        WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 500,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create window.", APP_NAME, MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiToken);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiToken);
    return (int)msg.wParam;
}
