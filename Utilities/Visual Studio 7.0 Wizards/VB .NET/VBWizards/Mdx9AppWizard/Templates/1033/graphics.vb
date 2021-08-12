Imports System
Imports System.Windows.Forms
Imports System.Drawing

'/ <summary>
'/ This class is where the Drawing routines
'/ for non-Direct3D and non-DirectDraw 
'/ applications reside.
'/ </summary>
Public Class GraphicsClass
    Private spriteSize As Single = 50
    Private owner As Control = Nothing
    Private graphicsWindow As Graphics = Nothing
    Public destination As Point = New Point(10, 10)

    Public Sub New(ByVal owner As Control)
        Me.owner = owner
        AddHandler owner.Paint, AddressOf PaintEvent
    End Sub 'New

    Public Sub RenderGraphics(ByVal destination As Point)
        Me.destination = destination
        owner.Refresh()
    End Sub 'RenderGraphics

    Public Sub PaintEvent(ByVal sender As Object, ByVal e As PaintEventArgs)
        e.Graphics.Clear(Color.Blue)
        e.Graphics.FillEllipse(Brushes.Black, destination.X, destination.Y, spriteSize, spriteSize)
    End Sub
End Class 'GraphicsClass
