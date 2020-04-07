//-----------------------------------------------------------------------------
// File: Text3D.cs
//
// Desc: Example code showing how to do text in a Direct3D scene.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
using System;
using System.Drawing;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Direct3D=Microsoft.DirectX.Direct3D;




namespace Text3D
{
	/// <summary>
	/// Application class. The base class (GraphicsSample) provides the generic 
	/// functionality needed in all Direct3D samples. MyGraphicsSample adds 
	/// functionality specific to this sample program.
	/// </summary>
	public class MyGraphicsSample : GraphicsSample
	{
        // Interop to call get device caps
        private const int LogPixelsY = 90;
        [System.Runtime.InteropServices.DllImport("gdi32.dll")]
        private static extern int GetDeviceCaps(IntPtr hdc, int cap);

		private Mesh mesh3DText = null;  // Mesh to draw 3d text
        private Direct3D.Font firstFont = null;
        private Direct3D.Font secondFont = null;
        private Direct3D.Font statsFont = null;
        private Sprite ourSprite = null;

        private System.Drawing.Font ourFont = new System.Drawing.Font("Arial", 12.0f, FontStyle.Bold);
		private Matrix objectOne = new Matrix();
		private Matrix objectTwo = new Matrix();

		private System.Windows.Forms.MenuItem mnuOptions;
		private System.Windows.Forms.MenuItem mnuChangeFont;

        private const string FontName = "Arial";
        private const int FontSize = 12;
        private const int FontSize2 = 9;

        private const string BigText = "The Font class supports word wrapping.  If you resize the window, the words " +
            "will automatically wrap to the next line.  It also supports\r\nhard line breaks.  Font also supports " +
            "Unicode text with correct word wrapping for right-to-left languages.";
        
        /// <summary>
		/// Application constructor. Sets attributes for the app.
		/// </summary>
		public MyGraphicsSample()
		{
			// Set the window text
			this.Text = "Text3D: Text in a 3D scene";
            try
            {
                // Load the icon from our resources
                System.Resources.ResourceManager resources = new System.Resources.ResourceManager(this.GetType());
                this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            }
            catch
            {
                // It's no big deal if we can't load our icons, but try to load the embedded one
                try { this.Icon = new System.Drawing.Icon(this.GetType(), "directx.ico"); } 
                catch {}
            }

			// Create our font objects
			enumerationSettings.AppUsesDepthBuffer = true;

			// Add our new menu options
			mnuOptions = new System.Windows.Forms.MenuItem("&Options");
			mnuChangeFont = new System.Windows.Forms.MenuItem("&Change Font...");
			// Add to the main menu screen
			mnuMain.MenuItems.Add(this.mnuOptions);
			mnuOptions.MenuItems.Add(mnuChangeFont);
			mnuChangeFont.Shortcut = System.Windows.Forms.Shortcut.CtrlO;
			mnuChangeFont.ShowShortcut = true;
			mnuChangeFont.Click += new System.EventHandler(this.ChangeFontClick);
		}



        /// <summary>
        /// Load the new fonts that should be used
        /// </summary>
        /// <param name="fontName">Name of the font to use</param>
        private void LoadFonts(string fontName)
        {
            int height;
            int logPixels;
            // Initialize all of the fonts
            using(Graphics g = this.CreateGraphics())
            {
                System.IntPtr dc = g.GetHdc();
                logPixels = GetDeviceCaps(dc, LogPixelsY);
                g.ReleaseHdc(dc);
            }

            height = -FontSize * logPixels / 72;

            firstFont = new Direct3D.Font(device, height, 0, FontWeight.Bold,
                0, false, CharacterSet.Default, Precision.Default, FontQuality.Default, 
                PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);

            height = -FontSize2 * logPixels / 72;
            secondFont = new Direct3D.Font(device, height, 0, FontWeight.Bold,
                0, false, CharacterSet.Default, Precision.Default, FontQuality.Default, 
                PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);
        }


        /// <summary>
        /// Handles font change
        /// </summary>
        private void ChangeFontClick(object sender, EventArgs e)
		{
			System.Windows.Forms.FontDialog dlg = new System.Windows.Forms.FontDialog();

			// Show the dialog
			dlg.FontMustExist = true;
			dlg.Font = ourFont;
			
			if (dlg.ShowDialog(this) == System.Windows.Forms.DialogResult.OK) // We selected something
			{
				ourFont = dlg.Font;
				if (firstFont != null)
					firstFont.Dispose();
                if (secondFont != null)
                    secondFont.Dispose();

				// Set the new font
                LoadFonts(ourFont.Name);

				// Create our 3d text mesh
				mesh3DText = Mesh.TextFromFont(device, ourFont, "Mesh.TextFromFont", 0.001f, 0.4f);
			}
		}




		/// <summary>
		/// Called once per frame, the call is the entry point for animating the scene.
		/// </summary>
		protected override void FrameMove()
		{
			// Setup five rotation matrices (for rotating text strings)
			Vector3 vAxis1 = new Vector3(1.0f,2.0f,0.0f);
			Vector3 vAxis2 = new Vector3(2.0f,1.0f,0.0f);
			objectOne = Matrix.RotationAxis(vAxis1, appTime / 2.0f);
			objectTwo = Matrix.RotationAxis(vAxis2, appTime / 2.0f);

			// Add some translational values to the matrices
			objectOne.M41 = 1.0f;   objectOne.M42 = 6.0f;   objectOne.M43 = 20.0f; 
			objectTwo.M41 = -4.0f;  objectTwo.M42 = -1.0f;  objectTwo.M43 = 0.0f; 
		}




		/// <summary>
		/// Called once per frame, the call is the entry point for 3d rendering. This 
		/// function sets up render states, clears the viewport, and renders the scene.
		/// </summary>
		protected override void Render()
		{
			// Clear the viewport
			device.Clear(ClearFlags.Target | ClearFlags.ZBuffer, Color.Black, 1.0f, 0);

			device.BeginScene();

            // Demonstration 1
            // Draw a simple line
            firstFont.DrawText(null, "Font.DrawText", new Rectangle(150,200, 0, 0), DrawTextFormat.NoClip, Color.Red);

            // Demonstration 2
            // Allow multiple draw calls to sort by texture changes by Sprite
            // When drawing 2D text use flags: SpriteFlags.AlphaBlend | SpriteFlags.SortTexture
            // When drawing 3D text use flags: SpriteFlags.AlphaBlend | SpriteFlags.SortDepthBackToFront
            ourSprite.Begin(SpriteFlags.AlphaBlend | SpriteFlags.SortTexture);
            secondFont.DrawText(ourSprite, "Font now caches letters on one or more textures.", 
                new Rectangle(10, presentParams.BackBufferHeight - 15*5, 0, 0), DrawTextFormat.NoClip,
                Color.White);

            secondFont.DrawText(ourSprite, "In order to sort by texture state changes on muliple", 
                new Rectangle(10, presentParams.BackBufferHeight - 15*4, 0, 0), DrawTextFormat.NoClip,
                Color.White);

            secondFont.DrawText(ourSprite, "draw calls to DrawText() pass a Sprite and use", 
                new Rectangle(10, presentParams.BackBufferHeight - 15*3, 0, 0), DrawTextFormat.NoClip,
                Color.White);

            secondFont.DrawText(ourSprite, "flags SpriteFlags.AlphaBlend | SpriteFlags.SortTexture", 
                new Rectangle(10, presentParams.BackBufferHeight - 15*2, 0, 0), DrawTextFormat.NoClip,
                Color.White);

            ourSprite.End();

            // Demonstration 3:
            // Word wrapping and unicode text.  
            // Note that not all fonts support dynamic font linking. 
            Rectangle rc = new Rectangle(10, 60, presentParams.BackBufferWidth, presentParams.BackBufferHeight);
            secondFont.DrawText(null, BigText, rc, 
                DrawTextFormat.Left | DrawTextFormat.WordBreak | DrawTextFormat.ExpandTabs,
                Color.Purple);

            // Output statistics
			statsFont.DrawText(null, frameStats, new Rectangle(2, 0, 0, 0), DrawTextFormat.NoClip, Color.Yellow);
            statsFont.DrawText(null, deviceStats, new Rectangle(2, 15, 0, 0), DrawTextFormat.NoClip, Color.Yellow);

			// Draw D3DXFont mesh in 3D (blue)
			if (mesh3DText != null)
			{
				Material mtrl3d = GraphicsUtility.InitMaterial(Color.Blue);
				device.Material = mtrl3d;
				device.Transform.World = objectTwo;
				mesh3DText.DrawSubset(0);
			}

			device.EndScene();
		}




		/// <summary>
        /// The device has been created.  Resources that are not lost on
        /// Reset() can be created here -- resources in Pool.Managed,
        /// Pool.Scratch, or Pool.SystemMemory.  Image surfaces created via
        /// CreateImageSurface are never lost and can be created here.  Vertex
        /// shaders and pixel shaders can also be created here as they are not
        /// lost on Reset().
		/// </summary>
		protected override void InitializeDeviceObjects()
		{
            LoadFonts(FontName);

            int height;
            int logPixels;
            // Initialize all of the fonts
            using(Graphics g = this.CreateGraphics())
            {
                System.IntPtr dc = g.GetHdc();
                logPixels = GetDeviceCaps(dc, LogPixelsY);
                g.ReleaseHdc(dc);
            }

            height = -FontSize * logPixels / 72;

            statsFont = new Direct3D.Font(device, height, 0, FontWeight.Bold,
                0, false, CharacterSet.Default, Precision.Default, FontQuality.Default, 
                PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, FontName);

            ourSprite = new Sprite(device);
		}




		/// <summary>
        /// The device exists, but may have just been Reset().  Resources in
        /// Pool.Default and any other device state that persists during
        /// rendering should be set here.  Render states, matrices, textures,
        /// etc., that don't change during rendering can be set once here to
        /// avoid redundant state setting during Render() or FrameMove().
		/// </summary>
		protected override void RestoreDeviceObjects(System.Object sender, System.EventArgs e)
		{
			// Restore the textures
			device.TextureState[0].ColorArgument1 = TextureArgument.TextureColor;
			device.TextureState[0].ColorArgument2 = TextureArgument.Diffuse;
			device.TextureState[0].ColorOperation = TextureOperation.Modulate;
			device.SamplerState[0].MinFilter = TextureFilter.Linear;
			device.SamplerState[0].MagFilter = TextureFilter.Linear;

			device.RenderState.ZBufferEnable = true;
			device.RenderState.DitherEnable = true;
			device.RenderState.SpecularEnable = true;
			device.RenderState.Lighting = true;
			device.RenderState.Ambient = System.Drawing.Color.FromArgb(unchecked((int)0x80808080));

			GraphicsUtility.InitLight(device.Lights[0], LightType.Directional, 10.0f, -10.0f, 10.0f);
			device.Lights[0].Commit();
			device.Lights[0].Enabled = true;

			// Set the transform matrices
			Vector3 vEyePt = new Vector3(0.0f,-5.0f,-10.0f);
			Vector3 vLookatPt = new Vector3(0.0f, 0.0f,  0.0f);
			Vector3 vUpVec = new Vector3(0.0f, 1.0f,  0.0f);
			Matrix matWorld, matView, matProj;

			matWorld = Matrix.Identity;
			matView = Matrix.LookAtLH(vEyePt, vLookatPt, vUpVec);
			float fAspect = device.PresentationParameters.BackBufferWidth / (float)device.PresentationParameters.BackBufferHeight;
			matProj = Matrix.PerspectiveFovLH((float)Math.PI / 4, fAspect, 1.0f, 100.0f);

			device.Transform.World = matWorld;
			device.Transform.View = matView;
			device.Transform.Projection = matProj;

			if (mesh3DText != null)
				mesh3DText.Dispose();

			// Create our 3d text mesh
			mesh3DText = Mesh.TextFromFont(device, ourFont, "Mesh.TextFromFont", 0.001f, 0.4f);
		}




        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        static void Main() 
        {
            using (MyGraphicsSample d3dApp = new MyGraphicsSample())
            {                                 
                if (d3dApp.CreateGraphicsSample())
                    d3dApp.Run();
            }
        }
    }
}
