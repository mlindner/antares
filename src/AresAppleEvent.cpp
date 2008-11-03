/*
Ares, a tactical space combat game.
Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/******************************************\
|**| Ares_AppleEvent.c
\******************************************/

#include "AresAppleEvent.h"

#include "AresExternalFile.h"
#include "AresGlobalType.h"
#include "Debug.h"
#include "Error.h"
#include "Processor.h"
#include "WrapGameRanger.h"

#pragma mark **DEFINITIONS**
/******************************************\
|**| #defines
\******************************************/

/* - definitions
*******************************************/

#define kAEInitErr                  76
#pragma mark _macros_
/* - macros
*******************************************/

#pragma mark **TYPEDEFS**
/******************************************\
|**| typedefs
\******************************************/

#pragma mark **EXTERNAL GLOBALS**
/******************************************\
|**| external globals
\******************************************/
extern aresGlobalType           *gAresGlobal;

#pragma mark **PRIVATE GLOBALS**
/******************************************\
|**| private globals
\******************************************/

#pragma mark **PRIVATE PROTOTYPES**
/******************************************\
|**| private function prototypes
\******************************************/

pascal OSErr HandleOApp( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon);

pascal OSErr HandleQuit( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon);

pascal OSErr HandlePrint( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon);

pascal OSErr HandleAnswer( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon);

pascal OSErr HandleOpenDoc( AppleEvent *theAppleEvent, AppleEvent *reply,
        long refcon);

pascal OSErr GotRequiredParams( AppleEvent *theAppleEvent);

#pragma mark **PRIVATE FUNCTIONS**
/******************************************\
|**| private functions
\******************************************/

pascal OSErr HandleOApp( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon)

{
    OSErr   error;

#pragma unused( reply, refcon)
//  error = GotRequiredParams( theAppleEvent);
//  if ( error != noErr) return ( error);

    if ( gAresGlobal->useGameRanger)
    {
        if ( Wrap_GRCheckAEForCmd( theAppleEvent))
        {
            gAresGlobal->gameRangerPending = true;
            gAresGlobal->returnToMain = true;
        }
    }

    error = GotRequiredParams( theAppleEvent);
    if ( error != noErr) return ( error);
    // no action

    return( noErr);
}

pascal OSErr HandleQuit( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon)

{
    OSErr   error;

#pragma unused( reply, refcon)

    error = GotRequiredParams( theAppleEvent);
    if ( error != noErr) return ( error);

//  DoQuit( true);
    gAresGlobal->returnToMain = true;
    gAresGlobal->isQuitting = true;

    return( noErr);
}

pascal OSErr HandlePrint( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon)
{
    OSErr error;

#pragma unused( reply, refcon)

    error = GotRequiredParams( theAppleEvent);
    if ( error != noErr) return ( error);

    // no action

    return( noErr);
}

pascal OSErr HandleAnswer( AppleEvent *theAppleEvent, AppleEvent *reply, long refcon)
{
    OSErr error = noErr;

#pragma unused( theAppleEvent, reply, refcon)

//  error = GotRequiredParams( theAppleEvent);
    if ( error != noErr)
    {
        return ( error);
    }

    // no action

    return( noErr);
}

pascal OSErr HandleOpenDoc( AppleEvent *theAppleEvent, AppleEvent *reply,
        long refcon)
{
    OSErr       error, ignoreError;
    FSSpec      myFSS;
    AEDescList  docList;
    long        index, itemsInList;
    Size        actualSize;
    AEKeyword   keywd;
    DescType    returnedType;

#pragma unused( reply, refcon)

    //    {get the direct parameter--a descriptor list--and put it into doclist
    error = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
    if ( error == noErr)
    {
        // check for missing params
        error = GotRequiredParams( theAppleEvent);
        if ( error == noErr)
        {
            // count the # of descriptor records in list
            error = AECountItems (&docList, &itemsInList);
            if ( error == noErr)
            {
                // get each descriptor from list
                // coerce into FSSpec record
                // and open
                for ( index = 1; index <= itemsInList; index++)
                {
                    error = AEGetNthPtr(&docList, index, typeFSS,
                                           &keywd, &returnedType, &myFSS,
                                           sizeof(myFSS), &actualSize);
                    if ( error == noErr)
                    {
                        if ( gAresGlobal->okToOpenFile)
                        {
                            gAresGlobal->originalExternalFileSpec = myFSS;
//                          SysBeep(20);
//                          EF_OpenExternalFile( );
                        }
/*                      DisposeTimerWindow( (WindowPtr)gTimerGlobal->window);
                        DisposeInfoWindow( (WindowPtr)gTimerGlobal->infoWindow);
                        ShowHideFloater(0);
                        UpdateFloater(0);

                        PPPTSaveAllPreferences( gTimerGlobal->state.customPrefsSpec);
                        InitTimerState();
                        gTimerGlobal->state.prefsSpec = myFSS;
                        gTimerGlobal->state.customPrefsSpec = &gTimerGlobal->state.prefsSpec;
                        ReadNormalPrefs();

                        NewPrefsStartup( false);
*/                  }
                }
            }
        } else
        {
            ignoreError = AEDisposeDesc( &docList);
        }
    } else
    {
        return( error);
    }
    return( error);
}


// see IM:VI p6-48
pascal OSErr GotRequiredParams( AppleEvent *theAppleEvent)

{
    DescType    theType;
    Size        theSize;
    OSErr       error;

    error = AEGetAttributePtr( theAppleEvent, keyMissedKeywordAttr, typeWildCard, &theType, nil,
            0, &theSize);
    if (error == errAEDescNotFound) // all required params gotten
    {
        WriteDebugLine((char *)"\pOK:errAEDescNotFound");
        return( noErr);
    }
    else if ( error == noErr) // missed param, because missed keyword exists
    {
        WriteDebugLine((char *)"\pBAD:miised keyword");
        return( errAEEventNotHandled);
    }
    else
    {
        WriteDebugLine((char *)"\pBAD:error");
        WriteDebugLong( error);
        return( error);
    }
}

#pragma mark **PUBLIC FUNCTIONS**
/******************************************\
|**| public functions
\******************************************/

OSErr AAE_Init( void)
{
    AEEventHandlerUPP       upp = nil;
    OSErr                   err;

    upp = NewAEEventHandlerProc( (AEEventHandlerProcPtr)HandleOApp);
    err = AEInstallEventHandler( kCoreEventClass, kAEOpenApplication,
            upp, 0, false);
    if ( err != noErr)
    {
        ShowErrorOfTypeOccurred( eQuitErr, kErrorStrID, kAEInitErr, err, __FILE__, 0);
        return err;
    }
    upp = NewAEEventHandlerProc( (AEEventHandlerProcPtr)HandleQuit);
    err = AEInstallEventHandler( kCoreEventClass, kAEQuitApplication,
            upp, 0, false);
    if ( err != noErr)
    {
        ShowErrorOfTypeOccurred( eQuitErr, kErrorStrID, kAEInitErr, err, __FILE__, 1);
        return err;
    }

    upp = NewAEEventHandlerProc( (AEEventHandlerProcPtr)HandlePrint);
    err = AEInstallEventHandler( kCoreEventClass, kAEPrint,
            upp, 0, false);
    if ( err != noErr)
    {
        ShowErrorOfTypeOccurred( eQuitErr, kErrorStrID, kAEInitErr, err, __FILE__, 2);
        return err;
    }

    upp = NewAEEventHandlerProc( (AEEventHandlerProcPtr)HandleOpenDoc);
    err = AEInstallEventHandler( kCoreEventClass, kAEOpenDocuments,
            upp, 0, false);
        if ( err != noErr)
    {
        ShowErrorOfTypeOccurred( eQuitErr, kErrorStrID, kAEInitErr, err, __FILE__, 3);
        return err;
    }

//  upp = NewAEEventHandlerProc( (ProcPtr)HandleAnswer);
//  err = AEInstallEventHandler( kCoreEventClass, kAEAnswer, upp, 0, false);
//      if ( err != noErr)
//  {
//      ShowErrorOfTypeOccurred( eQuitErr, kErrorStrID, kAEInitErr, err, __FILE__, 4);
//      return err;
//  }

    gAresGlobal->aeInited = true;
    return noErr;
}

