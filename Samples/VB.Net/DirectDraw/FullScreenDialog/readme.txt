//-----------------------------------------------------------------------------
// 
// Sample Name: FullScreenDialog Sample
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========
  FullScreenDialog demonstrates how to display a GDI dialog while using DirectDraw
  in full-screen exclusive mode. 

Path
====
   Source:     DXSDK\Samples\VB.NET\DirectDraw\FullScreenDialog
   Executable: DXSDK\Samples\VB.NET\DirectDraw\Bin

User's Guide
============
  FullScreenDialog requires no user input. Press the ESC key to quit the program.

Programming Notes
=================
  For details on how to setup a full-screen DirectDraw app, see the FullScreenMode 
  sample. 
  
  If the display device supports CanRenderWindowed then to make GDI write to 
  a DirectDraw surface, call Device.FlipToGDISurface, then create a clipper object
  which GDI uses when drawing.  To display a dialog, then simply create and show it 
  as normal.
    
  If the display device does not support CanRenderWindowed (some secondary 
  graphics cards are like this) then the card does not support FlipToGDISurface.  
  So it is necessary instead to have GDI make a bitmap of the dialog to be drawn, 
  then blt this bitmap to the back buffer.  However, this sample does not show how to
  do this technique,
  


