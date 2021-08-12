//-----------------------------------------------------------------------------
// 
// Sample Name: AdjustSound Sample
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
// 
//-----------------------------------------------------------------------------


Description
===========
  AdjustSound sample shows how to load and play a wave file using
  a DirectSound buffer and adjust its focus, frequency, pan, and volume.
  
Path
====
  Source:     DXSDK\Samples\C#\DirectSound\AdjustSound
  Executable: DXSDK\Samples\C#\DirectSound\Bin

User's Guide
============
  Load a wave file by clicking Sound File. Select Focus and Buffer Mixing 
  options; note that the various settings are explained under Expected Behavior 
  as you select them. Click Play. If you don't hear any sound, check the Status 
  pane. The application might fail to create the buffer in hardware if this 
  option has been selected. 

  By using the sliders you can adjust the frequency, pan, and volume dynamically 
  as the buffer is playing.

Programming Notes
=================
  For a simpler example of how to setup a DirectSound buffer without as many 
  controls, see the PlaySound sample. 
  
  To set the focus of a buffer create a new buffer with GlobalFocus or StickyFocus
  or neither of these flags. 
    
  To set the memory location of a buffer create a new buffer with LocHardware or LocSoftware
  or neither of these flags. 
    
  To control various aspects of DirectSound buffer:
    To adjust the frequency use Frequency property
    To adjust the pan use Pan property
    To adjust the volume use Volume property
    
    
  
  