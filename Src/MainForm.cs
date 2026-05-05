using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
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

    [DllImport("user32.dll")]
    static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

    [DllImport("user32.dll")]
    static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    public MainForm()
    {
        InitializeComponent();

        // Load Icon for Titlebar and Taskbar[cite: 1]
        try
        {
            string iconPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SVC_Appicon.ico");
            if (File.Exists(iconPath))
            {
                this.Icon = new Icon(iconPath);
            }
        }
        catch { /* Fallback if icon is corrupt */ }

        _playbackTimer = new System.Windows.Forms.Timer { Interval = 1000 };
        _playbackTimer.Tick += (s, e) => {
            if (_isPlaying && tkTimeline.Value < tkTimeline.Maximum)
            {
                tkTimeline.Value++;
                UpdateLabels();
            }
            else if (tkTimeline.Value >= tkTimeline.Maximum)
            {
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

    private void SetupGrid()
    {
        dgvCuts.Columns.Clear();
        dgvCuts.AutoGenerateColumns = false;
        dgvCuts.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "StartTime", HeaderText = "Start", Width = 90 });
        dgvCuts.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = "EndTime", HeaderText = "End", Width = 90 });
        dgvCuts.Columns.Add(new DataGridViewButtonColumn { Name = "PreviewBtn", Text = "Preview", UseColumnTextForButtonValue = true, HeaderText = "Preview" });
        dgvCuts.Columns.Add(new DataGridViewButtonColumn { Name = "DeleteBtn", Text = "Delete", UseColumnTextForButtonValue = true, HeaderText = "Delete" });

        dgvCuts.CellClick += (s, e) => {
            if (e.RowIndex < 0 || e.RowIndex >= _clips.Count) return;
            if (dgvCuts.Columns[e.ColumnIndex].Name == "PreviewBtn") SeekToTime(TimeSpan.Parse(_clips[e.RowIndex].StartTime).TotalSeconds);
            else if (dgvCuts.Columns[e.ColumnIndex].Name == "DeleteBtn") _clips.RemoveAt(e.RowIndex);
        };
    }

    private void UpdateLabels()
    {
        lblElapsed.Text = TimeSpan.FromSeconds(tkTimeline.Value).ToString(@"hh\:mm\:ss");
        double remaining = Math.Max(0, _durationSeconds - tkTimeline.Value);
        lblTotalTime.Text = "-" + TimeSpan.FromSeconds(remaining).ToString(@"hh\:mm\:ss");
    }

    private async void btnBrowse_Click(object? sender, EventArgs e)
    {
        using OpenFileDialog ofd = new() { Filter = "Video Files|*.mp4;*.mkv;*.avi;*.mov" };
        if (ofd.ShowDialog() == DialogResult.OK)
        {
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
        try
        {
            _ffplayProcess?.Kill();
            ProcessStartInfo psi = new("ffplay.exe", $"-ss {timestamp} -i \"{_inputPath}\" -noborder -x {pnlPreview.Width} -y {pnlPreview.Height} -autoexit")
            { CreateNoWindow = true, UseShellExecute = false };
            _ffplayProcess = Process.Start(psi);
            if (_isPlaying) _playbackTimer.Start();
            btnPlayPause.Text = "⏸ Pause";

            Task.Delay(500).ContinueWith(_ => {
                if (_ffplayProcess != null && !_ffplayProcess.HasExited)
                    this.Invoke(() => {
                        SetParent(_ffplayProcess.MainWindowHandle, pnlPreview.Handle);
                        MoveWindow(_ffplayProcess.MainWindowHandle, 0, 0, pnlPreview.Width, pnlPreview.Height, true);
                    });
            });
            _isPlaying = true;
        }
        catch { MessageBox.Show("ffplay.exe missing."); }
    }

    private async Task<double> GetVideoDuration(string path)
    {
        try
        {
            var psi = new ProcessStartInfo("ffprobe.exe", $"-v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"{path}\"")
            { RedirectStandardOutput = true, UseShellExecute = false, CreateNoWindow = true };
            using var process = Process.Start(psi);
            return double.Parse((await process!.StandardOutput.ReadToEndAsync()).Trim());
        }
        catch { return 0; }
    }

    private void SeekRelative(int secs) => SeekToTime(Math.Clamp(tkTimeline.Value + secs, 0, tkTimeline.Maximum));

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
        foreach (var clip in _clips)
        {
            string cleanStart = clip.StartTime.Replace(":", "-");
            string cleanEnd = clip.EndTime.Replace(":", "-");
            string output = Path.Combine(sourceDir, $"{baseName}_{cleanStart}_{cleanEnd}{ext}");
            
            string args = $"-ss {clip.StartTime} -to {clip.EndTime} -i \"{_inputPath}\" -c copy -y \"{output}\"";
            await Task.Run(() => Process.Start(new ProcessStartInfo("ffmpeg.exe", args) { CreateNoWindow = true })?.WaitForExit());
        }
        btnCutAll.Enabled = true;
        ShowCompletionDialog(sourceDir);
    }

    private void ShowCompletionDialog(string path)
    {
        Form diag = new Form {
            Text = "Status", Size = new Size(350, 150), 
            StartPosition = FormStartPosition.CenterParent, 
            FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false, MinimizeBox = false
        };
        Label lbl = new Label { Text = "All lossless cuts are completed.", Dock = DockStyle.Top, Height = 50, TextAlign = ContentAlignment.MiddleCenter };
        Button btn = new Button { Text = "Open Folder", Dock = DockStyle.Bottom, Height = 40 };
        btn.Click += (s, e) => { Process.Start("explorer.exe", path); diag.Close(); };
        diag.Controls.Add(lbl); diag.Controls.Add(btn);
        diag.ShowDialog();
    }
}