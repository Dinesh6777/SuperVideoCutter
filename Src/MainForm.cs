using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO.Compression; 
using System.Net.Http;
using System.Reflection; 
using System.Runtime.InteropServices;

namespace SuperVideoCutter;

public partial class MainForm : Form
{
    private string _inputPath = string.Empty;
    private Process? _ffplayProcess;
    private readonly BindingList<VideoClip> _clips = [];
    private double _durationSeconds = 0;
    private bool _isPlaying = false;
    private System.Windows.Forms.Timer _playbackTimer;

    private string _ffmpegPath = "ffmpeg.exe";
    private string _ffplayPath = "ffplay.exe";
    private string _ffprobePath = "ffprobe.exe";
    private readonly string _pluginsDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SuperVideoCutter_plugins");

    [DllImport("user32.dll")]
    static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
    [DllImport("user32.dll")]
    static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    public MainForm()
    {
        InitializeComponent();
        SetAppIcon(); 

        this.Shown += async (s, e) => await InitializeDependenciesAsync();

        _playbackTimer = new System.Windows.Forms.Timer { Interval = 1000 };
        _playbackTimer.Tick += (s, e) => {
            if (_isPlaying && tkTimeline.Value < tkTimeline.Maximum) {
                tkTimeline.Value++;
                UpdateLabels();
            } else if (tkTimeline.Value >= tkTimeline.Maximum) {
                StopPlayback();
            }
        };

        dgvCuts.DataSource = _clips;
        SetupGrid();
        this.KeyPreview = true;

        this.Resize += (s, e) => {
            if (_ffplayProcess != null && !_ffplayProcess.HasExited)
                MoveWindow(_ffplayProcess.MainWindowHandle, 0, 0, pnlPreview.Width, pnlPreview.Height, true);
        };
    }

    private void SetAppIcon()
    {
        try 
        {
            Assembly assembly = Assembly.GetExecutingAssembly();
            using (Stream? stream = assembly.GetManifestResourceStream("SuperVideoCutter.SVC_Appicon.ico"))
            {
                if (stream != null) this.Icon = new Icon(stream);
            }
        }
        catch { }
    }

    private async Task InitializeDependenciesAsync()
    {
        bool ffmpegOk = CheckAndSetToolPath(ref _ffmpegPath, "ffmpeg.exe");
        bool ffplayOk = CheckAndSetToolPath(ref _ffplayPath, "ffplay.exe");
        bool ffprobeOk = CheckAndSetToolPath(ref _ffprobePath, "ffprobe.exe");

        if (!ffmpegOk || !ffplayOk || !ffprobeOk) {
            var result = MessageBox.Show("FFmpeg tools are missing. Download them now?", "Setup", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if (result == DialogResult.Yes) {
                await DownloadAndExtractWithProgressAsync();
                CheckAndSetToolPath(ref _ffmpegPath, "ffmpeg.exe");
                CheckAndSetToolPath(ref _ffplayPath, "ffplay.exe");
                CheckAndSetToolPath(ref _ffprobePath, "ffprobe.exe");
            }
        }
    }

    private bool CheckAndSetToolPath(ref string pathVar, string toolName)
    {
        string pluginPath = Path.Combine(_pluginsDir, toolName);
        if (File.Exists(pluginPath)) { pathVar = pluginPath; return true; }
        string localPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, toolName);
        if (File.Exists(localPath)) { pathVar = localPath; return true; }
        return ExistsInSystemPath(toolName);
    }

    private bool ExistsInSystemPath(string tool)
    {
        try {
            using var p = Process.Start(new ProcessStartInfo("where", tool) { CreateNoWindow = true, UseShellExecute = false, RedirectStandardOutput = true });
            p?.WaitForExit();
            return p?.ExitCode == 0;
        } catch { return false; }
    }

    private async Task DownloadAndExtractWithProgressAsync()
    {
        string url = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip";
        string zipFile = Path.Combine(Path.GetTempPath(), "ffmpeg_temp.zip");

        using Form progressForm = new Form {
            Text = "Setting up tools", Size = new Size(400, 160),
            StartPosition = FormStartPosition.CenterParent, FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false, MinimizeBox = false
        };
        Label lblStatus = new Label { Text = "Downloading FFmpeg...", Location = new Point(20, 20), AutoSize = true };
        ProgressBar pb = new ProgressBar { Location = new Point(20, 55), Size = new Size(340, 25), Maximum = 100 };
        progressForm.Controls.AddRange(new Control[] { lblStatus, pb });

        progressForm.Shown += async (s, e) => {
            try {
                if (!Directory.Exists(_pluginsDir)) Directory.CreateDirectory(_pluginsDir);
                using HttpClient client = new HttpClient();
                using var response = await client.GetAsync(url, HttpCompletionOption.ResponseHeadersRead);
                var totalBytes = response.Content.Headers.ContentLength ?? -1L;
                using var stream = await response.Content.ReadAsStreamAsync();
                using var fileStream = new FileStream(zipFile, FileMode.Create, FileAccess.Write, FileShare.None, 8192, true);
                var buffer = new byte[8192];
                var totalRead = 0L;
                int bytesRead;
                while ((bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length)) > 0) {
                    await fileStream.WriteAsync(buffer, 0, bytesRead);
                    totalRead += bytesRead;
                    if (totalBytes != -1) pb.Invoke(() => pb.Value = (int)((totalRead * 100) / totalBytes));
                }
                fileStream.Close();
                lblStatus.Invoke(() => lblStatus.Text = "Extracting tools...");
                pb.Invoke(() => { pb.Value = 0; pb.Style = ProgressBarStyle.Marquee; });
                await Task.Run(() => {
                    using ZipArchive archive = ZipFile.OpenRead(zipFile);
                    foreach (var entry in archive.Entries) {
                        if (entry.Name == "ffmpeg.exe" || entry.Name == "ffplay.exe" || entry.Name == "ffprobe.exe")
                            entry.ExtractToFile(Path.Combine(_pluginsDir, entry.Name), true);
                    }
                });
                if (File.Exists(zipFile)) File.Delete(zipFile);
                progressForm.Close();
            } catch (Exception ex) {
                progressForm.Close();
                MessageBox.Show($"Setup failed: {ex.Message}");
            }
        };
        progressForm.ShowDialog();
    }

    private void SetupGrid()
    {
        dgvCuts.Columns.Clear();
        dgvCuts.AutoGenerateColumns = false;
        dgvCuts.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "StartTime", HeaderText = "Start", Width = 90 });
        dgvCuts.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "EndTime", HeaderText = "End", Width = 90 });
        dgvCuts.Columns.Add(new DataGridViewButtonColumn { Name = "PreviewBtn", Text = "Preview", UseColumnTextForButtonValue = true });
        dgvCuts.Columns.Add(new DataGridViewButtonColumn { Name = "DeleteBtn", Text = "Delete", UseColumnTextForButtonValue = true });
        dgvCuts.CellClick += (s, e) => {
            if (e.RowIndex < 0 || e.RowIndex >= _clips.Count) return;
            if (dgvCuts.Columns[e.ColumnIndex].Name == "PreviewBtn") SeekToTime(TimeSpan.Parse(_clips[e.RowIndex].StartTime).TotalSeconds);
            else if (dgvCuts.Columns[e.ColumnIndex].Name == "DeleteBtn") _clips.RemoveAt(e.RowIndex);
        };
    }

    private void UpdateLabels()
    {
        // Elapsed time on the left
        lblElapsed.Text = TimeSpan.FromSeconds(tkTimeline.Value).ToString(@"hh\:mm\:ss");
        // Total video duration on the right formatted as (00:00:00)[cite: 16]
        lblTotalTime.Text = "(" + TimeSpan.FromSeconds(_durationSeconds).ToString(@"hh\:mm\:ss") + ")";
    }

    private async void btnBrowse_Click(object? sender, EventArgs e)
    {
        using OpenFileDialog ofd = new() { Filter = "Video Files|*.mp4;*.mkv;*.avi;*.mov" };
        if (ofd.ShowDialog() == DialogResult.OK) {
            _inputPath = ofd.FileName;
            lblFile.Text = Path.GetFileName(_inputPath);
            _durationSeconds = await GetVideoDuration(_inputPath);
            tkTimeline.Maximum = (int)_durationSeconds;
            tkTimeline.Value = 0;
            UpdateLabels();
            StartFFplay("00:00:00");
        }
    }

    private void SeekToTime(double seconds)
    {
        tkTimeline.Value = (int)seconds;
        UpdateLabels();
        StartFFplay(TimeSpan.FromSeconds(tkTimeline.Value).ToString(@"hh\:mm\:ss"));
    }

    private void StopPlayback()
    {
        try { _ffplayProcess?.Kill(); } catch { }
        _playbackTimer.Stop();
        btnPlayPause.Text = "▶ Play";
        _isPlaying = false;
    }

    private void TogglePlayPause()
    {
        if (_isPlaying) StopPlayback();
        else StartFFplay(lblElapsed.Text);
    }

    private void StartFFplay(string timestamp)
    {
        if (string.IsNullOrEmpty(_inputPath)) return;
        try {
            _ffplayProcess?.Kill();
            ProcessStartInfo psi = new(_ffplayPath, $"-ss {timestamp} -i \"{_inputPath}\" -noborder -x {pnlPreview.Width} -y {pnlPreview.Height} -autoexit")
            { CreateNoWindow = true, UseShellExecute = false };
            _ffplayProcess = Process.Start(psi);
            
            if (_isPlaying) _playbackTimer.Start();
            btnPlayPause.Text = "⏸ Pause";

            // Robust Parenting Logic
            Task.Run(() => {
                Stopwatch sw = Stopwatch.StartNew();
                while (sw.ElapsedMilliseconds < 3000) {
                    if (_ffplayProcess != null && !_ffplayProcess.HasExited && _ffplayProcess.MainWindowHandle != IntPtr.Zero) {
                        this.Invoke(() => {
                            SetParent(_ffplayProcess.MainWindowHandle, pnlPreview.Handle);
                            MoveWindow(_ffplayProcess.MainWindowHandle, 0, 0, pnlPreview.Width, pnlPreview.Height, true);
                        });
                        break;
                    }
                    Thread.Sleep(100);
                }
            });
            _isPlaying = true;
        } catch { MessageBox.Show("ffplay missing."); }
    }

    private async Task<double> GetVideoDuration(string path)
    {
        try {
            var psi = new ProcessStartInfo(_ffprobePath, $"-v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"{path}\"")
            { RedirectStandardOutput = true, UseShellExecute = false, CreateNoWindow = true };
            using var process = Process.Start(psi);
            return double.Parse((await process!.StandardOutput.ReadToEndAsync()).Trim());
        } catch { return 0; }
    }

    private void SeekRelative(int secs) => SeekToTime(Math.Clamp(tkTimeline.Value + secs, 0, tkTimeline.Maximum));

    // Improved Timeline Interaction with accurate mouse calculation[cite: 16]
    private void tkTimeline_MouseDown(object? sender, MouseEventArgs e)
    {
        if (string.IsNullOrEmpty(_inputPath) || tkTimeline.Maximum <= 0) return;
        
        // Accurate calculation for point-and-click seeking
        double mousePos = e.X - 10; 
        double trackWidth = tkTimeline.Width - 20; 
        double ratio = mousePos / trackWidth;
        
        int newValue = (int)(ratio * tkTimeline.Maximum);
        tkTimeline.Value = Math.Clamp(newValue, 0, tkTimeline.Maximum);
        SeekToTime(tkTimeline.Value);
    }

    // Explicit marker handlers[cite: 16]
    private void btnSetStart_Click(object? sender, EventArgs e) => txtStart.Text = lblElapsed.Text;
    private void btnSetEnd_Click(object? sender, EventArgs e) => txtEnd.Text = lblElapsed.Text;

    protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
    {
        if (string.IsNullOrEmpty(_inputPath)) return base.ProcessCmdKey(ref msg, keyData);
        if (keyData == Keys.Space) { TogglePlayPause(); return true; }
        if (keyData == Keys.Left) { SeekRelative(-5); return true; }
        if (keyData == Keys.Right) { SeekRelative(5); return true; }
        return base.ProcessCmdKey(ref msg, keyData);
    }

    private async void btnCutAll_Click(object? sender, EventArgs e)
    {
        if (string.IsNullOrEmpty(_inputPath) || _clips.Count == 0) return;
        string sourceDir = Path.GetDirectoryName(_inputPath)!;
        string baseName = Path.GetFileNameWithoutExtension(_inputPath);
        string ext = Path.GetExtension(_inputPath);
        btnCutAll.Enabled = false;
        foreach (var clip in _clips) {
            string output = Path.Combine(sourceDir, $"{baseName}_{clip.StartTime.Replace(":", "-")}_{clip.EndTime.Replace(":", "-")}{ext}");
            string args = $"-ss {clip.StartTime} -to {clip.EndTime} -i \"{_inputPath}\" -c copy -y \"{output}\"";
            await Task.Run(() => Process.Start(new ProcessStartInfo(_ffmpegPath, args) { CreateNoWindow = true })?.WaitForExit());
        }
        btnCutAll.Enabled = true;
        ShowCompletionDialog(sourceDir);
    }

    private void ShowCompletionDialog(string path)
    {
        Form d = new Form { Text = "Done", Size = new Size(300, 150), StartPosition = FormStartPosition.CenterParent, FormBorderStyle = FormBorderStyle.FixedDialog };
        Label l = new Label { Text = "All lossless cuts completed.", Dock = DockStyle.Top, TextAlign = ContentAlignment.MiddleCenter, Height = 50 };
        Button b = new Button { Text = "Open Folder", Dock = DockStyle.Bottom, Height = 40 };
        b.Click += (s, e) => { Process.Start("explorer.exe", path); d.Close(); };
        d.Controls.Add(l); d.Controls.Add(b);
        d.ShowDialog();
    }
}