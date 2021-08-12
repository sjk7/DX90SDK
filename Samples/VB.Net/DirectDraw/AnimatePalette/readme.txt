//-----------------------------------------------------------------------------
// 
// Sample Name: AnimatePalette Sample
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========
  AnimatePalette demonstrates DirectDraw palette animation when in full-screen 
  on a palettized surface. 

Path
====
   Source:     DXSDK\Samples\VB.NET\DirectDraw\AnimatePalette
   Executable: DXSDK\Samples\VB.NET\DirectDraw\Bin

User's Guide
============
  AnimatePalette requires no user input. Press the ESC key to quit the program.

Programming Notes
=================
  For details on how to setup a full-screen DirectDraw app, see the FullScreenMode 
  sample. 
  
  To animate the palette on a palettized DirectDraw surface, call 
  Palette.GetEntries to retrieve the palette colors.  Then every 
  frame (or as often as desired) alter this array as needed, then set the new palette 
  by first calling   Device.WaitForVerticalBlank to synchronize the palette change to a 
  vertical blank, then call Palette.SetEntries.
  
  