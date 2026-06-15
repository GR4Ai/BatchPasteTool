#r "System.Drawing.Common"

using System.Drawing;
using System.Drawing.Imaging;

string srcPath = @"C:\Users\Administrator\Desktop\BatchPasteTool\src\BatchPasteTool\Resources\pin.png";
string dstPath = @"C:\Users\Administrator\Desktop\BatchPasteTool\src\BatchPasteTool\Resources\pin_filled.png";

using var bmp = new Bitmap(srcPath);
int w = bmp.Width, h = bmp.Height;

// Flood fill interior with black, starting from center of pin head area
// Pin icon: head is roughly in top half, center of head ~ (w/2, h/3)
int startX = w / 2;
int startY = h / 3;

// Get the outline color (should be nearly black)
Color outlineColor = bmp.GetPixel(startX, 0); // might not work, try a known outline pixel

// Simple flood fill using stack
var filled = new bool[w, h];
var stack = new Stack<Point>();
stack.Push(new Point(startX, startY));

while (stack.Count > 0)
{
    var p = stack.Pop();
    if (p.X < 0 || p.X >= w || p.Y < 0 || p.Y >= h) continue;
    if (filled[p.X, p.Y]) continue;
    
    var pixel = bmp.GetPixel(p.X, p.Y);
    // Stop at non-transparent pixels (outline) - alpha > 128 means not transparent
    if (pixel.A > 128) continue;
    
    filled[p.X, p.Y] = true;
    bmp.SetPixel(p.X, p.Y, Color.FromArgb(255, 26, 26, 26)); // #1A1A1A fill
    
    stack.Push(new Point(p.X - 1, p.Y));
    stack.Push(new Point(p.X + 1, p.Y));
    stack.Push(new Point(p.X, p.Y - 1));
    stack.Push(new Point(p.X, p.Y + 1));
}

bmp.Save(dstPath, ImageFormat.Png);
Console.WriteLine($"Created pin_filled.png ({w}x{h})");
