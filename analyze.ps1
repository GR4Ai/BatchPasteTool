Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Image]::FromFile("C:\Users\Administrator\Desktop\BatchPasteTool\ref_full.png")
$bmp = New-Object System.Drawing.Bitmap($img)
$W = $img.Width; $H = $img.Height

# From previous: left edge ~80, top edge ~110

# Find exact top of window (first white pixel after shadow gradient)
Write-Host "=== Finding exact window edges ==="
# Scan from x=80..90 at various y to find the window border
for ($ox = 0; $ox -lt 20; $ox++) {
    $found = $false
    for ($y = 100; $y -lt 130; $y++) {
        $px = $bmp.GetPixel(80+$ox, $y)
        if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
            $found = $true
            Write-Host ("x=" + (80+$ox) + " y=" + $y + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
            if ($ox -eq 0) { break }
        }
    }
    if ($ox -ge 5 -and -not $found) { break }
}

# Scan entire title bar area horizontally at y=118 (inside window, below shadow)
Write-Host ""
Write-Host "=== Title bar full horizontal scan (y=118, every 5px from x=85) ==="
for ($x = 85; $x -lt 800; $x += 5) {
    $px = $bmp.GetPixel($x, 118)
    if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
        Write-Host ("x=" + $x + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
    }
}

# Find title bar bottom separator line
Write-Host ""
Write-Host "=== Finding title bar separator (vertical scan at x=200) ==="
for ($y = 115; $y -lt 170; $y++) {
    $px = $bmp.GetPixel(200, $y)
    if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
        Write-Host ("y=" + $y + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
    }
}

# Find first item row
Write-Host ""
Write-Host "=== Finding first content row below title bar ==="
for ($y = 140; $y -lt 250; $y++) {
    $px = $bmp.GetPixel(200, $y)
    if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
        Write-Host ("y=" + $y + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
        break
    }
}

# Find bottom bar area
Write-Host ""
Write-Host "=== Bottom area scan up from bottom ==="
for ($y = $H - 1; $y -gt $H - 200; $y--) {
    $px = $bmp.GetPixel(200, $y)
    if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
        Write-Host ("y=" + $y + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
    }
}

# Find bottom bar separator
Write-Host ""
Write-Host "=== Bottom bar separator search (y from h-200 to h-100 at x=200) ==="
for ($y = $H - 200; $y -lt $H - 100; $y++) {
    $px = $bmp.GetPixel(200, $y)
    if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
        Write-Host ("y=" + $y + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
    }
}

# Right edge scan
Write-Host ""
Write-Host "=== Right side window controls area (y=118, x from W-300 to W) ==="
for ($x = $W - 300; $x -lt $W; $x += 5) {
    $px = $bmp.GetPixel($x, 118)
    if ($px.R -ne 255 -or $px.G -ne 255 -or $px.B -ne 255) {
        Write-Host ("x=" + $x + ": R=" + $px.R + " G=" + $px.G + " B=" + $px.B)
    }
}

$bmp.Dispose()
$img.Dispose()
