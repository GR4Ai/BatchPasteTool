using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;

string srcPath = args.Length > 0 ? args[0] : @"Resources\pin.png";
string dstPath = args.Length > 1 ? args[1] : @"Resources\pin_filled.png";

using var bmp = new Bitmap(srcPath);
int w = bmp.Width, h = bmp.Height;

// Pin shape: the interior is the transparent region inside the outline.
// Flood fill from center of the pin head (top portion of the icon).
int startX = w / 2;
int startY = h / 3;

// BFS flood fill
var filled = new bool[w, h];
var queue = new Queue<Point>();
queue.Enqueue(new Point(startX, startY));

while (queue.Count > 0)
{
    var p = queue.Dequeue();
    if (p.X < 0 || p.X >= w || p.Y < 0 || p.Y >= h) continue;
    if (filled[p.X, p.Y]) continue;

    var pixel = bmp.GetPixel(p.X, p.Y);
    // Stop at outline pixels (alpha > 200, nearly opaque)
    if (pixel.A > 200) continue;

    filled[p.X, p.Y] = true;
    bmp.SetPixel(p.X, p.Y, Color.FromArgb(255, 26, 26, 26)); // #1A1A1A

    queue.Enqueue(new Point(p.X - 1, p.Y));
    queue.Enqueue(new Point(p.X + 1, p.Y));
    queue.Enqueue(new Point(p.X, p.Y - 1));
    queue.Enqueue(new Point(p.X, p.Y + 1));
}

bmp.Save(dstPath, ImageFormat.Png);
Console.WriteLine($"Created {dstPath} ({w}x{h})");
