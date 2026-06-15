namespace BatchPasteTool.Helpers;

public static class Constants
{
    // Window
    public const string AppName = "BatchPasteTool";
    public const string ConfigFile = "BatchPasteTool.json";

    // Layout (pixels)
    public const int TitleH = 32;
    public const int BottomH = 42;
    public const int ItemH = 40;
    public const int ItemGap = 4;
    public const int IconW = 24;
    public const int IconH = 24;
    public const int ScrollbarW = 16;
    public const int SliderW = 140;
    public const int SliderH = 18;
    public const int BtnW = 56;
    public const int BtnH = 30;
    public const int WinBtnW = 46;
    public const int WinBtnH = 32;
    public const int EditH = 26;
    public const int SendBtnW = 52;
    public const int ClearBtnW = 24;

    // Window limits
    public const int MinWinW = 420;
    public const int MinWinH = 220;
    public const int DefaultWinW = 620;
    public const int DefaultWinH = 500;

    // Transparency
    public const int TranspMin = 51;   // 80% transparent
    public const int TranspMax = 255;  // fully opaque
    public const int TranspToggle = 180; // semi-transparent toggle

    // Timers (ms)
    public const int AutoSaveMs = 3000;
    public const int ForegroundCheckMs = 300;

    // Limits
    public const int MaxItems = 100;
    public const int MaxUndo = 200;

    // Colors
    public const string ColTitleBg = "#FFFFFF";
    public const string ColTitleText = "#1A1A1A";
    public const string ColContentBg = "#FFFFFF";
    public const string ColItemBg = "#F0F0F0";
    public const string ColSeparator = "#BEBEBE";
    public const string ColTitleSeparator = "#C8C8C8";
    public const string ColBottomSeparator = "#B4B4B4";
    public const string ColWindowBorder = "#B4B4B4";
    public const string ColSendBtn = "#326EDC";
    public const string ColSendBtnHover = "#1450B4";
    public const string ColSendBtnText = "#FFFFFF";
    public const string ColCloseHover = "#E81123";
    public const string ColMinMaxHover = "#E0E0E0";
    public const string ColSliderFill = "#4285F4";
    public const string ColSliderTrack = "#B4B4B4";
    public const string ColSliderEmpty = "#C8C8C8";
    public const string ColClearX = "#828282";
    public const string ColClearXHover = "#DC3C32";
    public const string ColBtnText = "#326EDC";
    public const string ColBtnTextHover = "#1450B4";
    public const string ColBottomBg = "#FFFFFF";
    public const string ColPinActive = "#4285F4";
    public const string ColIconHover = "#644285F4";

    // Fonts
    public const string FontFamily = "Segoe UI";
    public const double TitleFontSize = 14.0;
    public const double SendFontSize = 12.0;
    public const double BtnFontSize = 12.0;
    public const double EditFontSize = 18.0;
}
