========================================================================
    CONSOLE APPLICATION : ray Project Overview
========================================================================

AppWizard has created this ray application for you.  

This file contains a summary of what you will find in each of the files that
make up your ray application.


ray.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard. 
    It contains information about the version of Visual C++ that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

ray.cpp
    This is the main application source file.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named ray.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

Changes:

=> Adding Nuget package for openGL (glm.h, udray.h)
=> New method to calculate ray intersection with triangles (udray.cpp)
=> Adding code for calculate ray intersection with sphere (ray.cpp)
=> Output depth information to depthInfo.bin file (ray.cpp, udray.cpp write_DPT)
=> File format:
w : XXX, h: XXX // Image width and height
XXX XXX XXX .... // depth value, normalized to 0 - 255, 255 for background.