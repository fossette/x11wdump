/*
 * File:        x11wdump.c
 *
 * Author:      fossette
 *
 * Date:        2019/05/20
 *
 * Version:     1.0
 *
 * Description: Dump all the X11 windows and (some of) their attributes
 *              into a file.  To prevent an overwelming high number of
 *              windows, the WDUMP_MAXLEVEL constant limits how much deep
 *              the search goes.  Tested under FreeBSD 11.2 using
 *              libX11 1.6.7. Should be easy to port because there are
 *              almost no dependencies.
 *
 * Parameter:   None.
 *
 * Web:         https://github.com/fossette/x11wdump/wiki
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vlc/vlc.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>




/*
 *  Constants
 */

#define ERROR_WDUMP_SYS    1
#define ERROR_WDUMP_X11    2

#define LNSZ               200

#define WDUMP_MAXLEVEL     10




/*
 *  Global variable
 */

char gszErr[LNSZ];




void
Event2sz(long iEvMask,     char *pEvent)
{
   *pEvent = 0;


   if (iEvMask & KeyPressMask)
      strcpy(pEvent, "KeyPrs");
   if (iEvMask & KeyReleaseMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "KeyRel");
   }
   if (iEvMask & ButtonPressMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "ButPrs");
   }
   if (iEvMask & ButtonReleaseMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "ButRel");
   }
   if (iEvMask & EnterWindowMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "EntWin");
   }
   if (iEvMask & LeaveWindowMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "LeavWin");
   }
   if (iEvMask & PointerMotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "PntrMot");
   }
   if (iEvMask & PointerMotionHintMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "PntrMotHint");
   }
   if (iEvMask & Button1MotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "But1Mot");
   }
   if (iEvMask & Button2MotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "But2Mot");
   }
   if (iEvMask & Button3MotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "But3Mot");
   }
   if (iEvMask & Button4MotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "But4Mot");
   }
   if (iEvMask & Button5MotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "But5Mot");
   }
   if (iEvMask & ButtonMotionMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "ButMot");
   }
   if (iEvMask & KeymapStateMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "KeymapState");
   }
   if (iEvMask & ExposureMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "Expos");
   }
   if (iEvMask & VisibilityChangeMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "VisChng");
   }
   if (iEvMask & StructureNotifyMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "StrNot");
   }
   if (iEvMask & ResizeRedirectMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "ResizRedir");
   }
   if (iEvMask & SubstructureNotifyMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "SubstrNot");
   }
   if (iEvMask & SubstructureRedirectMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "SubstrRedir");
   }
   if (iEvMask & FocusChangeMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "FocChng");
   }
   if (iEvMask & PropertyChangeMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "PropChng");
   }
   if (iEvMask & ColormapChangeMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "CmapChng");
   }
   if (iEvMask & OwnerGrabButtonMask)
   {
      if (*pEvent)
         strcat(pEvent, "|");
      strcat(pEvent, "OwnGrbBut");
   }
}




int
DumpWindow(FILE *pFile, Display *pX11Display, Window w,
           Atom aName, Atom aTypeStr, int iLevel)
{
   int               i,
                     iErr = 0,
                     iFormat,
                     iRet;
   unsigned int      nChildren;
   unsigned long     iNumItems,
                     iByteRemaining;
   char              szClass[LNSZ],
                     szEvent[LNSZ],
                     szName[LNSZ],
                     szSpace[LNSZ],
                     szState[LNSZ];
   unsigned char     *pProperty = NULL;
   Atom              aRet;
   Window            wParent,
                     wRoot,
                     *pChildren = NULL;
   XWindowAttributes attrib;


   *szName = 0;
   // Prepare the indentation space
   i = iLevel * 2;
   szSpace[i] = 0;
   while (i)
   {
      i--;
      szSpace[i] = ' ';
   }

   if (!XGetWindowAttributes(pX11Display, w,     &attrib))
   {
      iErr = ERROR_WDUMP_X11;
      sprintf(gszErr, "XGetWindowAttributes(w:0x%lX) failed!", w);
   }
   if (!iErr)
   {
      Event2sz(attrib.all_event_masks,     szEvent);

      iRet = XGetWindowProperty(pX11Display, w, aName, 0, 4, 0, aTypeStr,
                &aRet, &iFormat, &iNumItems, &iByteRemaining, &pProperty);
      if (iRet == Success && pProperty)
      {
         strncpy(szName, (char *)pProperty, LNSZ-1);
         szName[LNSZ-1] = 0;
      }
      if (pProperty)
         XFree(pProperty);

      iRet = XQueryTree(pX11Display, w,     &wRoot, &wParent,
                                            &pChildren, &nChildren);

      if (attrib.class == InputOutput)
         strcpy(szClass, "InputOutput");
      else if (attrib.class == InputOnly)
         strcpy(szClass, "InputOnly");
      else
         sprintf(szClass, "%d", attrib.class);
      
      if (attrib. map_state == IsUnmapped)
         strcpy(szState, "IsUnmapped");
      else if (attrib. map_state == IsUnviewable)
         strcpy(szState, "IsUnviewable");
      else if (attrib. map_state == IsViewable)
         strcpy(szState, "IsViewable");
      else
         sprintf(szState, "%d", attrib. map_state);
      
      fprintf(pFile, "%sw:0x%lX ", szSpace, w);
      if (*szName)
         fprintf(pFile, "NAME:%s ", szName);
      fprintf(pFile, "x:%d y:%d width:%d h:%d parent:0x%lX root:0x%lX"
                     " class:%s state:%s allEvents:0x%lX(%s)"
                     " yourEvents:0x%lX overrideRedirect:%d\n",
              attrib.x, attrib.y, attrib.width, attrib.height,
              wParent, attrib.root, szClass, szState,
              attrib.all_event_masks, szEvent, attrib.your_event_mask,
              attrib.override_redirect);

      // Same thing on children
      if (iLevel < WDUMP_MAXLEVEL && iRet)
      {
         if (pChildren && nChildren)
            for (i = 0 ; i < nChildren && !iErr ; i++)
               iErr = DumpWindow(pFile, pX11Display, pChildren[i],
                                 aName, aTypeStr, iLevel + 1);
      }
      if (pChildren)
         XFree(pChildren);
   }

   return (iErr);
}





int
main(int argc, char* argv[])
{
   int         iErr = 0;
   char        sz[LNSZ];
   time_t      iCurrTime;
   Atom        aName,
               aTypeStr;
   Display     *pX11Display = NULL;
   FILE        *pFile = NULL;
   Window      wRoot;
   struct tm   *pTm;

 
   *gszErr = 0;
   time(     &iCurrTime);
   pTm = localtime(&iCurrTime);
   if (!pTm)
   {
      iErr = ERROR_WDUMP_SYS;
      strcat(gszErr, "localtime() failed!");
   }
   if (!iErr)
   {
      sprintf(sz, "x11wdump-%d.%02d.%02d-%dh%02d.txt",
               pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
               pTm->tm_hour, pTm->tm_min);
      pFile = fopen(sz, "w");
      if (!pFile)
      {
         iErr = ERROR_WDUMP_SYS;
         sprintf(gszErr, "fopen(%s) failed!", sz);
      }
   }
   if (!iErr)
   {
      printf("File: %s\n", sz);
      fprintf(pFile, "File: %s\n\n", sz);
      
      pX11Display = XOpenDisplay(NULL);
      if (!pX11Display)
      {
         iErr = ERROR_WDUMP_X11;
         strcat(gszErr, "XOpenDisplay() failed!");
      }
   }
   if (!iErr)
   {
      wRoot = XDefaultRootWindow(pX11Display);
      fprintf(pFile, "Root: 0x%lX\n\n", wRoot);

      aName = XInternAtom(pX11Display, "WM_NAME", False);
      aTypeStr = XInternAtom(pX11Display, "STRING", False);
      if (aName == None || aTypeStr == None)
      {
         iErr = ERROR_WDUMP_X11;
         strcpy(gszErr, "XInternAtom() failed!");
      }
   }
   if (!iErr)
      iErr = DumpWindow(pFile, pX11Display, wRoot, aName, aTypeStr, 0);

   switch (iErr)
   {
      case ERROR_WDUMP_SYS:
         printf("SYSTEM ERROR: %s\n", gszErr);
         break;

      case ERROR_WDUMP_X11:
         printf("X11 ERROR: %s\n", gszErr);
         break;
   }
 
   if (pX11Display)
      XCloseDisplay(pX11Display);

   if (pFile)
   {
      fprintf(pFile, "\nDone.\n");
      fclose(pFile);
      printf("  Done.\n\n");
   }

   return 0;
}
