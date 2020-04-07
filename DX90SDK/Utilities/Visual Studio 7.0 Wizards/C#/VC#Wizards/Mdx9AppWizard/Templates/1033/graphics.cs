using System;
using System.Windows.Forms;
using System.Drawing;

/// <summary>
/// This class is where the Drawing routines
/// for non-Direct3D and non-DirectDraw 
/// applications reside.
/// </summary>
public class GraphicsClass
{
    private const float spriteSize = 50;
    private Control owner = null;
    private Point destination;

    public GraphicsClass(Control owner)
    {
        this.owner = owner;
        owner.Paint += new System.Windows.Forms.PaintEventHandler(this.PaintEvent);        
    }

    public void RenderGraphics(Point destination)
    {
        this.destination = destination;
        owner.Refresh();
    }

    private void PaintEvent(object sender, System.Windows.Forms.PaintEventArgs e)
    {
        e.Graphics.Clear(Color.Blue);
        e.Graphics.FillEllipse(Brushes.Black, destination.X, destination.Y, spriteSize, spriteSize);
    }
}