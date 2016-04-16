/******************************************************************************
*                                                                             *
*    resource.h                                  Copyright(c) 2009 itow,y.    *
*                                                                             *
******************************************************************************/

/******************************************************************************

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

******************************************************************************/


#define IDI_ICON	10

#define IDM_TRAY	20

#define CM_EXIT	100

#define IDS_ERR_CREATEMUTEX		1000
#define IDS_ERR_ALREADYRUNNING	1001
#define IDS_ERR_REGISTERCLASS	1002
#define IDS_ERR_CREATEWINDOW	1003
#define IDS_ERR_BEGINHOOK		1004
#define IDS_ERR_GETMESSAGE		1005
#define IDS_ERR_ADDTRAYICON		1006
#ifdef HOOK_DLL
#define IDS_ERR_HOOKDLLLOAD		1007
#define IDS_ERR_PROCADDRESS		1008
#endif
