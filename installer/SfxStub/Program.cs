using System.IO.Compression;
using System.Diagnostics;

namespace SfxStub;

class Program
{
    [STAThread]
    static void Main()
    {
        try
        {
            string exePath = Environment.ProcessPath!;

            // Read our own EXE and find the ZIP payload
            byte[] exeBytes = File.ReadAllBytes(exePath);

            // Find ZIP local file header signature PK\x03\x04
            int zipStart = -1;
            for (int i = 0; i < exeBytes.Length - 4; i++)
            {
                if (exeBytes[i] == 0x50 && exeBytes[i + 1] == 0x4B
                    && exeBytes[i + 2] == 0x03 && exeBytes[i + 3] == 0x04)
                {
                    zipStart = i;
                    break;
                }
            }

            if (zipStart < 0)
            {
                Console.WriteLine("ERROR: Installer is corrupted (ZIP signature not found).");
                Console.ReadLine();
                return;
            }

            // Extract ZIP to temp directory
            string tempDir = Path.Combine(Path.GetTempPath(), "BatchPasteTool_Install");
            if (Directory.Exists(tempDir))
                Directory.Delete(tempDir, true);
            Directory.CreateDirectory(tempDir);

            Console.Write("Extracting... ");
            using (var ms = new MemoryStream(exeBytes, zipStart, exeBytes.Length - zipStart))
            using (var zip = new ZipArchive(ms, ZipArchiveMode.Read))
            {
                ExtractZip(zip, tempDir);
            }
            Console.WriteLine("Done.");

            // Launch install script
            string installScript = Path.Combine(tempDir, "install.ps1");
            Console.WriteLine("Launching installer...");

            var psi = new ProcessStartInfo
            {
                FileName = "powershell.exe",
                Arguments = $"-ExecutionPolicy Bypass -File \"{installScript}\"",
                UseShellExecute = true,
                Verb = "runas"
            };

            var process = Process.Start(psi);
            process?.WaitForExit();

            // Cleanup temp
            try { Directory.Delete(tempDir, true); } catch { }

            Console.WriteLine("Setup complete.");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ERROR: {ex.Message}");
            Console.WriteLine(ex.StackTrace);
            Console.ReadLine();
        }
    }

    static void ExtractZip(ZipArchive archive, string destDir)
    {
        foreach (var entry in archive.Entries)
        {
            string destPath = Path.Combine(destDir, entry.FullName);
            string? dir = Path.GetDirectoryName(destPath);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                Directory.CreateDirectory(dir);
            if (!string.IsNullOrEmpty(entry.Name))
                entry.ExtractToFile(destPath, overwrite: true);
        }
    }
}
