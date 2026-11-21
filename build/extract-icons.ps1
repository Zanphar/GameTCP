Add-Type -AssemblyName System.Drawing
foreach ($file in @('GamerTCP.exe', 'GamerTCP-Setup.exe')) {
    $path = (Resolve-Path $file).Path
    $icon = [System.Drawing.Icon]::ExtractAssociatedIcon($path)
    $bitmap = $icon.ToBitmap()
    $name = [System.IO.Path]::GetFileNameWithoutExtension($file)
    $out = Join-Path (Get-Location) ("build\" + $name + "-icon.png")
    $bitmap.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Output ($name + " -> " + $out + " (" + $bitmap.Width + "x" + $bitmap.Height + ")")
}
