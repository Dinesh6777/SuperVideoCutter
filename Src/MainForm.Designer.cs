namespace SuperVideoCutter;

partial class MainForm
{
    private System.ComponentModel.IContainer components = null;
    private Panel pnlPreview;
    private TrackBar tkTimeline;
    private Label lblElapsed, lblTotalTime, lblFile;
    private Button btnPlayPause, btnBack5, btnForward5, btnBrowse;
    private Button btnSetStart, btnSetEnd, btnAddCut, btnCutAll;
    private TextBox txtStart, txtEnd;
    private DataGridView dgvCuts;
    private TableLayoutPanel tlpControls;

    private void InitializeComponent()
    {
        this.pnlPreview = new Panel { Dock = DockStyle.Top, Height = 350, BackColor = Color.Black };

        // Timeline Container
        Panel pnlTimeline = new Panel { Dock = DockStyle.Top, Height = 60 };
        this.lblElapsed = new Label { Text = "00:00:00", Location = new Point(5, 20), Width = 85, Font = new Font("Consolas", 10F) };
        this.tkTimeline = new TrackBar { Location = new Point(95, 12), Width = 440, Height = 45, TickStyle = TickStyle.None, Anchor = AnchorStyles.Left | AnchorStyles.Right };
        this.lblTotalTime = new Label { Text = "-00:00:00", Location = new Point(540, 20), Width = 100, Anchor = AnchorStyles.Right, Font = new Font("Consolas", 10F) };
        pnlTimeline.Controls.AddRange(new Control[] { lblElapsed, tkTimeline, lblTotalTime });

        // Control Grid with Padding
        this.tlpControls = new TableLayoutPanel { Dock = DockStyle.Top, Height = 145, ColumnCount = 4, RowCount = 2 };
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));
        this.tlpControls.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25F));

        // Added Padding via Margins
        this.btnBack5 = new Button { Text = "⏪ -5s", Dock = DockStyle.Fill, Margin = new Padding(8) };
        this.btnPlayPause = new Button { Text = "▶ Play", Dock = DockStyle.Fill, Margin = new Padding(12), Font = new Font("Segoe UI", 10F, FontStyle.Bold) };
        this.btnForward5 = new Button { Text = "+5s ⏩", Dock = DockStyle.Fill, Margin = new Padding(8) };
        this.btnBrowse = new Button { Text = "Browse Video", Dock = DockStyle.Fill, Margin = new Padding(12), BackColor = Color.Gainsboro };

        // Renamed Trim Buttons[cite: 1]
        Panel pnlStart = new Panel { Dock = DockStyle.Fill };
        this.btnSetStart = new Button { Text = "Mark Start", Width = 90, Height = 35, Location = new Point(5, 8) };
        this.txtStart = new TextBox { Text = "00:00:00", Location = new Point(100, 12), Width = 70 };
        pnlStart.Controls.AddRange(new Control[] { btnSetStart, txtStart });

        Panel pnlEnd = new Panel { Dock = DockStyle.Fill };
        this.btnSetEnd = new Button { Text = "Mark End", Width = 90, Height = 35, Location = new Point(5, 8) };
        this.txtEnd = new TextBox { Text = "00:00:00", Location = new Point(100, 12), Width = 70 };
        pnlEnd.Controls.AddRange(new Control[] { btnSetEnd, txtEnd });

        this.lblFile = new Label { Text = "No file", Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
        this.btnAddCut = new Button { Text = "Add to Cut", Width = 110, Height = 40, Anchor = AnchorStyles.None, BackColor = Color.LightSkyBlue };

        this.tlpControls.Controls.Add(btnBack5, 0, 0);
        this.tlpControls.Controls.Add(btnPlayPause, 1, 0);
        this.tlpControls.Controls.Add(btnForward5, 2, 0);
        this.tlpControls.Controls.Add(btnBrowse, 3, 0);
        this.tlpControls.Controls.Add(pnlStart, 0, 1);
        this.tlpControls.Controls.Add(pnlEnd, 1, 1);
        this.tlpControls.Controls.Add(lblFile, 2, 1);
        this.tlpControls.Controls.Add(btnAddCut, 3, 1);

        this.dgvCuts = new DataGridView { Dock = DockStyle.Fill, BackgroundColor = Color.White, AllowUserToAddRows = false, AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill };
        this.btnCutAll = new Button { Text = "PROCESS ALL LOSSLESS CUTS", Dock = DockStyle.Bottom, Height = 70, BackColor = Color.SteelBlue, ForeColor = Color.White, Font = new Font("Segoe UI", 11F, FontStyle.Bold) };

        this.Controls.Add(dgvCuts);
        this.Controls.Add(tlpControls);
        this.Controls.Add(pnlTimeline);
        this.Controls.Add(pnlPreview);
        this.Controls.Add(btnCutAll);

        this.Text = "Super Video Cutter Pro";
        this.Size = new Size(740, 980);
        this.StartPosition = FormStartPosition.CenterScreen;

        // Events
        this.btnBrowse.Click += btnBrowse_Click;
        this.btnSetStart.Click += (s, e) => txtStart.Text = lblElapsed.Text;
        this.btnSetEnd.Click += (s, e) => txtEnd.Text = lblElapsed.Text;
        this.btnBack5.Click += (s, e) => SeekRelative(-5);
        this.btnPlayPause.Click += (s, e) => TogglePlayPause();
        this.btnForward5.Click += (s, e) => SeekRelative(5);
        this.btnAddCut.Click += (s, e) => _clips.Add(new VideoClip { StartTime = txtStart.Text, EndTime = txtEnd.Text });
        this.btnCutAll.Click += btnCutAll_Click;
        this.tkTimeline.Scroll += (s, e) => { if(!string.IsNullOrEmpty(_inputPath)) SeekToTime(tkTimeline.Value); };
    }
}