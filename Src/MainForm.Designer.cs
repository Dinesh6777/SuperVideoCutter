namespace SuperVideoCutter;

partial class MainForm
{
    private Panel pnlPreview;
    private TrackBar tkTimeline;
    private Label lblElapsed, lblTotalTime, lblFile;
    private Button btnPlayPause, btnBack5, btnForward5, btnBrowse;
    private Button btnSetStart, btnSetEnd, btnAddCut, btnCutAll;
    private TextBox txtStart, txtEnd;
    private DataGridView dgvCuts;
    private TableLayoutPanel tlpControls;

    protected override void Dispose(bool disposing)
    {
        try { _ffplayProcess?.Kill(); } catch { }
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        this.pnlPreview = new Panel { Dock = DockStyle.Top, Height = 400, BackColor = Color.Black };

        Panel pnlTimeline = new Panel { Dock = DockStyle.Top, Height = 65 };
        // Smaller font for elapsed time label[cite: 15]
        this.lblElapsed = new Label { Text = "00:00:00", Location = new Point(5, 22), Width = 85, Font = new Font("Consolas", 8.25F) };
        this.tkTimeline = new TrackBar { Location = new Point(95, 12), Width = 550, Height = 45, TickStyle = TickStyle.None, Anchor = AnchorStyles.Left | AnchorStyles.Right };
        // Smaller font for total time label at the right side[cite: 15]
        this.lblTotalTime = new Label { Text = "(00:00:00)", Location = new Point(650, 22), Width = 100, Anchor = AnchorStyles.Right, Font = new Font("Consolas", 8.25F) };
        pnlTimeline.Controls.AddRange(new Control[] { lblElapsed, tkTimeline, lblTotalTime });

        this.tlpControls = new TableLayoutPanel { Dock = DockStyle.Top, Height = 170, ColumnCount = 4, RowCount = 2 };
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.RowStyles.Add(new RowStyle(SizeType.Absolute, 85F));
        this.tlpControls.RowStyles.Add(new RowStyle(SizeType.Absolute, 85F));

        this.btnBack5 = new Button { Text = "⏪ -5s", Dock = DockStyle.Fill };
        this.btnPlayPause = new Button { Text = "▶ Play", Dock = DockStyle.Fill, Font = new Font("Segoe UI", 10F, FontStyle.Bold) };
        this.btnForward5 = new Button { Text = "+5s ⏩", Dock = DockStyle.Fill };
        this.btnBrowse = new Button { Text = "Browse Video", Dock = DockStyle.Fill, BackColor = Color.Gainsboro };

        Panel pnlS = new Panel { Dock = DockStyle.Fill };
        this.btnSetStart = new Button { Text = "Mark Start", Width = 90, Height = 50, Location = new Point(5, 10) };
        this.txtStart = new TextBox { Text = "00:00:00", Location = new Point(100, 25), Width = 70 };
        pnlS.Controls.AddRange(new Control[] { btnSetStart, txtStart });

        Panel pnlE = new Panel { Dock = DockStyle.Fill };
        this.btnSetEnd = new Button { Text = "Mark End", Width = 90, Height = 50, Location = new Point(5, 10) };
        this.txtEnd = new TextBox { Text = "00:00:00", Location = new Point(100, 25), Width = 70 };
        pnlE.Controls.AddRange(new Control[] { btnSetEnd, txtEnd });

        this.lblFile = new Label { Text = "No file", Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
        this.btnAddCut = new Button { Text = "Add to Cut", Size = new Size(130, 50), Anchor = AnchorStyles.None, BackColor = Color.LightSkyBlue };

        this.tlpControls.Controls.Add(btnBack5, 0, 0);
        this.tlpControls.Controls.Add(btnPlayPause, 1, 0);
        this.tlpControls.Controls.Add(btnForward5, 2, 0);
        this.tlpControls.Controls.Add(btnBrowse, 3, 0);
        this.tlpControls.Controls.Add(pnlS, 0, 1);
        this.tlpControls.Controls.Add(pnlE, 1, 1);
        this.tlpControls.Controls.Add(lblFile, 2, 1);
        this.tlpControls.Controls.Add(btnAddCut, 3, 1);

        this.dgvCuts = new DataGridView { Dock = DockStyle.Fill, BackgroundColor = Color.White, AllowUserToAddRows = false, AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill };
        this.btnCutAll = new Button { Text = "PROCESS ALL LOSSLESS CUTS", Dock = DockStyle.Bottom, Height = 80, BackColor = Color.SteelBlue, ForeColor = Color.White, Font = new Font("Segoe UI", 12F, FontStyle.Bold) };

        this.Controls.Add(dgvCuts);
        this.Controls.Add(tlpControls);
        this.Controls.Add(pnlTimeline);
        this.Controls.Add(pnlPreview);
        this.Controls.Add(btnCutAll);

        this.Text = "SuperVideoCutter"; 
        this.Size = new Size(850, 950); 
        this.AutoScaleMode = AutoScaleMode.Dpi;
        this.StartPosition = FormStartPosition.CenterScreen;

        // Corrected Events[cite: 15]
        this.btnBrowse.Click += btnBrowse_Click;
        this.btnSetStart.Click += btnSetStart_Click;
        this.btnSetEnd.Click += btnSetEnd_Click;
        this.btnBack5.Click += (s, e) => SeekRelative(-5);
        this.btnPlayPause.Click += (s, e) => TogglePlayPause();
        this.btnForward5.Click += (s, e) => SeekRelative(5);
        this.btnAddCut.Click += (s, e) => _clips.Add(new VideoClip { StartTime = txtStart.Text, EndTime = txtEnd.Text });
        this.btnCutAll.Click += btnCutAll_Click;
        this.tkTimeline.Scroll += (s, e) => { if(!string.IsNullOrEmpty(_inputPath)) SeekToTime(tkTimeline.Value); };
        this.tkTimeline.MouseDown += tkTimeline_MouseDown;
    }
}