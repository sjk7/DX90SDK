'-----------------------------------------------------------------------------
' File: Text3D.cs
'
' Desc: Example code showing how to do text in a Direct3D scene.
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'-----------------------------------------------------------------------------
Imports System
Imports System.Drawing
Imports Microsoft.DirectX
Imports Microsoft.DirectX.Direct3D
Imports Direct3D = Microsoft.DirectX.Direct3D





'/ <summary>
'/ Application class. The base class (GraphicsSample) provides the generic 
'/ functionality needed in all Direct3D samples. MyGraphicsSample adds 
'/ functionality specific to this sample program.
'/ </summary>

Public Class MyGraphicsSample
    Inherits GraphicsSample
    ' Interop to call get device caps
    Private Const LogPixelsY As Integer = 90

    Private Declare Function GetDeviceCaps Lib "gdi32.dll" (ByVal hdc As IntPtr, ByVal cap As Integer) As Integer

    Private mesh3DText As Mesh = Nothing ' Mesh to draw 3d text
    Private firstFont As Direct3D.Font = Nothing
    Private secondFont As Direct3D.Font = Nothing
    Private statsFont As Direct3D.Font = Nothing
    Private ourSprite As Sprite = Nothing

    Private ourFont As New System.Drawing.Font("Arial", 12.0F, FontStyle.Bold)
    Private objectOne As New Matrix
    Private objectTwo As New Matrix

    Private mnuOptions As System.Windows.Forms.MenuItem
    Private mnuChangeFont As System.Windows.Forms.MenuItem

    Private Const FontName As String = "Arial"
    Private Const FontSize As Integer = 12
    Private Const FontSize2 As Integer = 9

    Private Const BigText As String = "The Font class supports word wrapping.  If you resize the window, the words " + "will automatically wrap to the next line.  It also supports" + vbCr + vbLf + "hard line breaks.  Font also supports " + "Unicode text with correct word wrapping for right-to-left languages."


        '/ <summary>
        '/ Application constructor. Sets attributes for the app.
        '/ </summary>
    Public Sub New()
        ' Set the window text
        Me.Text = "Text3D: Text in a 3D scene"
        Try
            ' Load the icon from our resources
            Dim resources As New System.Resources.ResourceManager(Me.GetType())
            Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Catch
            ' It's no big deal if we can't load our icons, but try to load the embedded one
            Try
                Me.Icon = New System.Drawing.Icon(Me.GetType(), "directx.ico")
            Catch
            End Try
        End Try
        ' Create our font objects
        enumerationSettings.AppUsesDepthBuffer = True

        ' Add our new menu options
        mnuOptions = New System.Windows.Forms.MenuItem("&Options")
        mnuChangeFont = New System.Windows.Forms.MenuItem("&Change Font...")
        ' Add to the main menu screen
        mnuMain.MenuItems.Add(Me.mnuOptions)
        mnuOptions.MenuItems.Add(mnuChangeFont)
        mnuChangeFont.Shortcut = System.Windows.Forms.Shortcut.CtrlO
        mnuChangeFont.ShowShortcut = True
        AddHandler mnuChangeFont.Click, AddressOf Me.ChangeFontClick
    End Sub 'New




    '/ <summary>
    '/ Load the new fonts that should be used
    '/ </summary>
    '/ <param name="fontName">Name of the font to use</param>
    Private Sub LoadFonts(ByVal fontName As String)
        Dim height As Integer
        Dim logPixels As Integer
        ' Initialize all of the fonts
        Dim g As Graphics = Me.CreateGraphics()
        Try
            Dim dc As System.IntPtr = g.GetHdc()
            logPixels = GetDeviceCaps(dc, LogPixelsY)
            g.ReleaseHdc(dc)
        Finally
            g.Dispose()
        End Try

        height = -FontSize * logPixels / 72

        firstFont = New Direct3D.Font(device, height, 0, FontWeight.Bold, 0, False, CharacterSet.Default, Precision.Default, FontQuality.Default, PitchAndFamily.DefaultPitch Or PitchAndFamily.FamilyDoNotCare, fontName)

        height = -FontSize2 * logPixels / 72
        secondFont = New Direct3D.Font(device, height, 0, FontWeight.Bold, 0, False, CharacterSet.Default, Precision.Default, FontQuality.Default, PitchAndFamily.DefaultPitch Or PitchAndFamily.FamilyDoNotCare, fontName)
    End Sub 'LoadFonts



    '/ <summary>
    '/ Handles font change
    '/ </summary>
    Private Sub ChangeFontClick(ByVal sender As Object, ByVal e As EventArgs)
        Dim dlg As New System.Windows.Forms.FontDialog

        ' Show the dialog
        dlg.FontMustExist = True
        dlg.Font = ourFont

        If dlg.ShowDialog(Me) = System.Windows.Forms.DialogResult.OK Then ' We selected something
            ourFont = dlg.Font
            If Not (firstFont Is Nothing) Then
                firstFont.Dispose()
            End If
            If Not (secondFont Is Nothing) Then
                secondFont.Dispose()
            End If
            ' Set the new font
            LoadFonts(ourFont.Name)

            ' Create our 3d text mesh
            mesh3DText = Mesh.TextFromFont(device, ourFont, "Mesh.TextFromFont", 0.001F, 0.4F)
        End If
    End Sub 'ChangeFontClick





    '/ <summary>
    '/ Called once per frame, the call is the entry point for animating the scene.
    '/ </summary>
    Protected Overrides Sub FrameMove()
        ' Setup five rotation matrices (for rotating text strings)
        Dim vAxis1 As New Vector3(1.0F, 2.0F, 0.0F)
        Dim vAxis2 As New Vector3(2.0F, 1.0F, 0.0F)
        objectOne = Matrix.RotationAxis(vAxis1, appTime / 2.0F)
        objectTwo = Matrix.RotationAxis(vAxis2, appTime / 2.0F)

        ' Add some translational values to the matrices
        objectOne.M41 = 1.0F
        objectOne.M42 = 6.0F
        objectOne.M43 = 20.0F
        objectTwo.M41 = -4.0F
        objectTwo.M42 = -1.0F
        objectTwo.M43 = 0.0F
    End Sub 'FrameMove





    '/ <summary>
    '/ Called once per frame, the call is the entry point for 3d rendering. This 
    '/ function sets up render states, clears the viewport, and renders the scene.
    '/ </summary>
    Protected Overrides Sub Render()
        ' Clear the viewport
        device.Clear(ClearFlags.Target Or ClearFlags.ZBuffer, Color.Black, 1.0F, 0)

        device.BeginScene()

        ' Demonstration 1
        ' Draw a simple line
        firstFont.DrawText(Nothing, "Font.DrawText", New Rectangle(150, 200, 0, 0), DrawTextFormat.NoClip, Color.Red)

        ' Demonstration 2
        ' Allow multiple draw calls to sort by texture changes by Sprite
        ' When drawing 2D text use flags: SpriteFlags.AlphaBlend | SpriteFlags.SortTexture
        ' When drawing 3D text use flags: SpriteFlags.AlphaBlend | SpriteFlags.SortDepthBackToFront
        ourSprite.Begin(SpriteFlags.AlphaBlend Or SpriteFlags.SortTexture)
        secondFont.DrawText(ourSprite, "Font now caches letters on one or more textures.", New Rectangle(10, presentParams.BackBufferHeight - 15 * 5, 0, 0), DrawTextFormat.NoClip, Color.White)

        secondFont.DrawText(ourSprite, "In order to sort by texture state changes on muliple", New Rectangle(10, presentParams.BackBufferHeight - 15 * 4, 0, 0), DrawTextFormat.NoClip, Color.White)

        secondFont.DrawText(ourSprite, "draw calls to DrawText() pass a Sprite and use", New Rectangle(10, presentParams.BackBufferHeight - 15 * 3, 0, 0), DrawTextFormat.NoClip, Color.White)

        secondFont.DrawText(ourSprite, "flags SpriteFlags.AlphaBlend | SpriteFlags.SortTexture", New Rectangle(10, presentParams.BackBufferHeight - 15 * 2, 0, 0), DrawTextFormat.NoClip, Color.White)

        ourSprite.End()

        ' Demonstration 3:
        ' Word wrapping and unicode text.  
        ' Note that not all fonts support dynamic font linking. 
        Dim rc As New Rectangle(10, 60, presentParams.BackBufferWidth, presentParams.BackBufferHeight)
        secondFont.DrawText(Nothing, BigText, rc, DrawTextFormat.Left Or DrawTextFormat.WordBreak Or DrawTextFormat.ExpandTabs, Color.Purple)

        ' Output statistics
        statsFont.DrawText(Nothing, frameStats, New Rectangle(2, 0, 0, 0), DrawTextFormat.NoClip, Color.Yellow)
        statsFont.DrawText(Nothing, deviceStats, New Rectangle(2, 15, 0, 0), DrawTextFormat.NoClip, Color.Yellow)

        ' Draw D3DXFont mesh in 3D (blue)
        If Not (mesh3DText Is Nothing) Then
            Dim mtrl3d As Material = GraphicsUtility.InitMaterial(Color.Blue)
            device.Material = mtrl3d
            device.Transform.World = objectTwo
            mesh3DText.DrawSubset(0)
        End If

        device.EndScene()
    End Sub 'Render





    '/ <summary>
    '/ The device has been created.  Resources that are not lost on
    '/ Reset() can be created here -- resources in Pool.Managed,
    '/ Pool.Scratch, or Pool.SystemMemory.  Image surfaces created via
    '/ CreateImageSurface are never lost and can be created here.  Vertex
    '/ shaders and pixel shaders can also be created here as they are not
    '/ lost on Reset().
    '/ </summary>
    Protected Overrides Sub InitializeDeviceObjects()
        LoadFonts(FontName)

        Dim height As Integer
        Dim logPixels As Integer
        ' Initialize all of the fonts
        Dim g As Graphics = Me.CreateGraphics()
        Try
            Dim dc As System.IntPtr = g.GetHdc()
            logPixels = GetDeviceCaps(dc, LogPixelsY)
            g.ReleaseHdc(dc)
        Finally
            g.Dispose()
        End Try

        height = -FontSize * logPixels / 72

        statsFont = New Direct3D.Font(device, height, 0, FontWeight.Bold, 0, False, CharacterSet.Default, Precision.Default, FontQuality.Default, PitchAndFamily.DefaultPitch Or PitchAndFamily.FamilyDoNotCare, FontName)

        ourSprite = New Sprite(device)
    End Sub 'InitializeDeviceObjects





    '/ <summary>
    '/ The device exists, but may have just been Reset().  Resources in
    '/ Pool.Default and any other device state that persists during
    '/ rendering should be set here.  Render states, matrices, textures,
    '/ etc., that don't change during rendering can be set once here to
    '/ avoid redundant state setting during Render() or FrameMove().
    '/ </summary>
    Protected Overrides Sub RestoreDeviceObjects(ByVal sender As System.Object, ByVal e As System.EventArgs)
        ' Restore the textures
        device.TextureState(0).ColorArgument1 = TextureArgument.TextureColor
        device.TextureState(0).ColorArgument2 = TextureArgument.Diffuse
        device.TextureState(0).ColorOperation = TextureOperation.Modulate
        device.SamplerState(0).MinFilter = TextureFilter.Linear
        device.SamplerState(0).MagFilter = TextureFilter.Linear

        device.RenderState.ZBufferEnable = True
        device.RenderState.DitherEnable = True
        device.RenderState.SpecularEnable = True
        device.RenderState.Lighting = True
        Device.RenderState.Ambient = System.Drawing.Color.FromArgb(&H80808080)

        GraphicsUtility.InitLight(device.Lights(0), LightType.Directional, 10.0F, -10.0F, 10.0F)
        device.Lights(0).Commit()
        device.Lights(0).Enabled = True

        ' Set the transform matrices
        Dim vEyePt As New Vector3(0.0F, -5.0F, -10.0F)
        Dim vLookatPt As New Vector3(0.0F, 0.0F, 0.0F)
        Dim vUpVec As New Vector3(0.0F, 1.0F, 0.0F)
        Dim matWorld, matView, matProj As Matrix

        matWorld = Matrix.Identity
        matView = Matrix.LookAtLH(vEyePt, vLookatPt, vUpVec)
        Dim fAspect As Single = device.PresentationParameters.BackBufferWidth / System.Convert.ToSingle(device.PresentationParameters.BackBufferHeight)
        matProj = Matrix.PerspectiveFovLH(System.Convert.ToSingle(Math.PI) / 4, fAspect, 1.0F, 100.0F)

        device.Transform.World = matWorld
        device.Transform.View = matView
        device.Transform.Projection = matProj

        If Not (mesh3DText Is Nothing) Then
            mesh3DText.Dispose()
        End If
        ' Create our 3d text mesh
        mesh3DText = Mesh.TextFromFont(device, ourFont, "Mesh.TextFromFont", 0.001F, 0.4F)
    End Sub 'RestoreDeviceObjects





    '/ <summary>
    '/ The main entry point for the application.
    '/ </summary>
    Shared Sub Main()
        Dim d3dApp As New MyGraphicsSample
        Try
            If d3dApp.CreateGraphicsSample() Then
                d3dApp.Run()
            End If
        Finally
            d3dApp.Dispose()
        End Try
    End Sub 'Main
End Class 'MyGraphicsSample