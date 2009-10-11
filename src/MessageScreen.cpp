// Ares, a tactical space combat game.
// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

// Message Screen.c -- also the destination screen
// must be initialized AFTER Screen Label.c

#include "MessageScreen.hpp"

#include "AnyChar.hpp"
#include "AresGlobalType.hpp"
#include "ColorTranslation.hpp"
#include "Debug.hpp"
#include "DirectText.hpp"
#include "Error.hpp"
#include "KeyMapTranslation.hpp"
#include "OffscreenGWorld.hpp"
#include "Options.hpp"
#include "PlayerInterface.hpp"  // for Replace_KeyCode_Strings_With_Actual_Key_Names
#include "PlayerInterfaceDrawing.hpp" // for long messages
#include "Resources.h"
#include "ScenarioMaker.hpp"
#include "ScreenLabel.hpp"
#include "SpriteHandling.hpp"

namespace antares {

#define kMessageScreenLeft      200
#define kMessageScreenTop       454
#define kMessageScreenRight     420
#define kMessageScreenBottom    475
#define kMessageScreenVCenter   ( kMessageScreenTop + (( kMessageScreenBottom - kMessageScreenTop) >> 1))
#define kMessageNullCharacter   ' '
#define kMessageSeparateChar    '~'
#define kMessageEndChar         '`'

#define kMaxMessageLength       2048
#define kLongOffsetFirstChar    0L      // offset in longwords to long word which is first char offset
#define kLongOffsetFirstFree    1L      // offset in longwords to long word which is first free char
#define kAnyCharOffsetStart     (( sizeof( long) * 2L) / sizeof(unsigned char))
                                        // offset in unsigned char to first data char
#define kAnyCharLastChar        ( kAnyCharOffsetStart + kMaxMessageLength * sizeof(unsigned char))
#define kMessageCharWidth       18      // width of message screen in characters (16?)

#define kMessageScreenError     "\pMSSG"

#define kMessageColor           RED
#define kMessageMoveTime        30
#define kLowerTime              (kMessageDisplayTime - kMessageMoveTime)
#define kRaiseTime              kMessageMoveTime
#define kMessageDisplayTime     (kMessageMoveTime * 2 + 120)

#define kDestinationLeft        12
#define kDestinationTop         162
#define kDestinationRight       122
#define kDestinationBottom      185
#define kDestinationVCenter     ( kDestinationTop + (( kDestinationBottom - kDestinationTop) >> 1))

#define kDestinationLength      127
#define kDestinationColor       ORANGE

#define kStatusLabelLeft        200
#define kStatusLabelTop         50
#define kStatusLabelAge         120

#define kMaxMessageLineNum      20
#define kMaxRetroSize           10
#define kLongMessageVPad        5
#define kLongMessageVPadDouble  10

#define kMessageCharTopBuffer       0
#define kMessageCharBottomBuffer    0
#define kReturnChar             0x0d
#define kCodeChar               '\\'
#define kCodeTabChar            't'
#define kCodeInvertChar         'i'
#define kCodeColorChar          'c'
#define kCodeRevertChar         'r'
#define kCodeForeColorChar      'f'
#define kCodeBackColorChar      'b'

#define kStringMessageID        1

#define kLongMessageFontNum     kTacticalFontNum
#define kUseMessage

inline int mHexDigitValue(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else {
        return c - 'a' + 10;
    }
}

template <typename T0, typename T1>
inline void mClipAnyRect(T0& mtrect, const T1& mclip) {
    mtrect.left = std::max(mtrect.left, mclip.left);
    mtrect.top = std::max(mtrect.top, mclip.top);
    mtrect.right = std::min(mtrect.right, mclip.right);
    mtrect.bottom = std::min(mtrect.bottom, mclip.bottom);
}

#define kHBuffer        4
#define kHBufferTotal   (kHBuffer)

extern directTextType   *gDirectText;
extern long             CLIP_LEFT, CLIP_RIGHT, CLIP_BOTTOM;

extern WindowPtr        gTheWindow;
extern long             gNatePortLeft, gNatePortTop, WORLD_HEIGHT, WORLD_WIDTH;
extern scenarioType     *gThisScenario; // for special message labels
extern PixMap*          gActiveWorld;
extern PixMap*          gOffWorld;

void MessageLabel_Set_Special(short id, TypedHandle<unsigned char> text);

int InitMessageScreen() {
    unsigned char *anyChar, nilLabel = 0;
    long            i;
    longMessageType *tmessage = nil;

    globals()->gTrueClipBottom = CLIP_BOTTOM;
    globals()->gMessageData.reset(new MessageData(kMaxMessageLength));
    globals()->gStatusString.reset(new unsigned char[kDestinationLength]);
    globals()->gLongMessageData.reset(new longMessageType);

    globals()->gMessageData->_first_char = 0;
    globals()->gMessageData->_first_free = 0;

    anyChar = globals()->gMessageData->_data.get();

    for (i = 0; i < kMaxMessageLength; i++) {
        *anyChar = kMessageEndChar;
        anyChar++;
    }

    globals()->gMessageLabelNum = AddScreenLabel( kMessageScreenLeft,
                        kMessageScreenTop, 0, 0,
                        &nilLabel, nil, false, kMessageColor);

    if ( globals()->gMessageLabelNum < 0)
    {
        ShowErrorAny( eQuitErr, kErrorStrID, nil, nil, nil, nil, kAddScreenLabelError, -1, -1, -1, __FILE__, 3);
        return( MEMORY_ERROR);
    }
    globals()->gStatusLabelNum = AddScreenLabel( kStatusLabelLeft, kStatusLabelTop, 0, 0,
                        &nilLabel, nil, false, kStatusLabelColor);
    if ( globals()->gStatusLabelNum < 0)
    {
        ShowErrorAny( eQuitErr, kErrorStrID, nil, nil, nil, nil, kAddScreenLabelError, -1, -1, -1, __FILE__, 4);
        return( MEMORY_ERROR);
    }

    tmessage = globals()->gLongMessageData.get();
    tmessage->startResID =  tmessage->endResID = tmessage->lastResID = tmessage->currentResID =
        -1;
    tmessage->time = 0;
    tmessage->stage = kNoStage;
    tmessage->textHeight = 0;
    tmessage->retroTextSpec.text = TypedHandle<unsigned char>();
    tmessage->retroTextSpec.textLength = 0;
    tmessage->retroTextSpec.thisPosition = 0;
    tmessage->charDelayCount = 0;
    tmessage->pictBounds.left = tmessage->pictBounds.right= 0;
    tmessage->pictCurrentLeft = 0;
    tmessage->pictCurrentTop = 0;
    tmessage->pictID = -1;
    tmessage->retroTextSpec.topBuffer = kMessageCharTopBuffer;
    tmessage->retroTextSpec.bottomBuffer = kMessageCharBottomBuffer;
    tmessage->stringMessage[0] = 0;
    tmessage->lastStringMessage[0] = 0;
    tmessage->newStringMessage = false;
    tmessage->labelMessage = false;
    tmessage->lastLabelMessage = false;
    tmessage->labelMessageID = -1;

    return kNoError;
}

void MessageScreenCleanup( void)

{
#ifdef kUseMessage
    globals()->gMessageData.reset();
    globals()->gStatusString.reset();
#endif
}

void ClearMessage( void)

{
#ifdef kUseMessage
    long    i;
    unsigned char *anyChar, nilLabel = 0;
    longMessageType *tmessage;

    globals()->gMessageData->_first_char = 0;
    globals()->gMessageData->_first_free = 0;
    anyChar = globals()->gMessageData->_data.get();
    for ( i = 0; i < kMaxMessageLength; i++)
        *anyChar++ = kMessageEndChar;
    globals()->gMessageTimeCount = 0;
    globals()->gMessageLabelNum = AddScreenLabel( kMessageScreenLeft, kMessageScreenTop, 0, 0,
                        &nilLabel, nil, false, kMessageColor);
    globals()->gStatusLabelNum = AddScreenLabel( kStatusLabelLeft, kStatusLabelTop, 0, 0,
                        &nilLabel, nil, false, kStatusLabelColor);

    tmessage = globals()->gLongMessageData.get();
    tmessage->startResID = -1;
    tmessage->endResID = -1;
    tmessage->currentResID = -1;
    tmessage->lastResID = -1;
    tmessage->textHeight = 0;
    tmessage->previousStartResID = tmessage->previousEndResID = -1;
    tmessage = globals()->gLongMessageData.get();
    tmessage->stringMessage[0] = 0;
    tmessage->lastStringMessage[0] = 0;
    tmessage->newStringMessage = false;
    tmessage->labelMessage = false;
    tmessage->lastLabelMessage = false;
    if (tmessage->retroTextSpec.text.get() != nil) {
        tmessage->retroTextSpec.text.destroy();
    }
    CLIP_BOTTOM = globals()->gTrueClipBottom;
    tmessage->labelMessageID = AddScreenLabel( 0, 0, 0, 0, &nilLabel, nil,
        false, SKY_BLUE);
    SetScreenLabelKeepOnScreenAnyway( tmessage->labelMessageID, true);

#endif
}

void AppendStringToMessage(const unsigned char* string) {
#ifdef kUseMessage
    unsigned char strLen, *message;
    int32_t *freeoffset;


    // get the offset to the first free character
    freeoffset = &globals()->gMessageData->_first_free;

    // set the destination char (message) to the first free character
    message = globals()->gMessageData->_data.get() + *freeoffset;

    // get the length of the source string
    strLen = *string++;

    // copy the string
    while (strLen != 0)
    {
        // copy the character
        *message++ = *string++;

        // increase the first free character offset
        (*freeoffset)++;

        // if the first free characrer offset == the length of our data (in chars) then wrap around
        if (*freeoffset >= kMaxMessageLength)
        {
            // reset offset to first char data (after the two longs of offsets)
            *freeoffset = kAnyCharOffsetStart;

            // reset the destination char to the first free char
            message = globals()->gMessageData->_data.get() + *freeoffset;
        }

        strLen--;
    }
#endif
}

void StartMessage( void) {
    unsigned char* message;
    int32_t *freeoffset;

    // get the offset to the first free character
    freeoffset = &globals()->gMessageData->_first_free;

    // set the destination char (message) to the first free character
    message = globals()->gMessageData->_data.get() + *freeoffset;

    // we should be on the special end char, which we turn into a separator char
    *message++ = kMessageSeparateChar;

    // increase the first free character offset
    (*freeoffset)++;

    // if the first free characrer offset == the length of our data (in chars) then wrap around
    if (implicit_cast<uint32_t>(*freeoffset) >= kMaxMessageLength) {
        // reset offset to first char data (after the two longs of offsets)
        *freeoffset = 0;

        // reset the destination char to the first free char
        message = globals()->gMessageData->_data.get() + *freeoffset;
    }
}

void EndMessage( void) {
    unsigned char* message;
    int32_t *freeoffset;

    // get the offset to the first free character
    freeoffset = &globals()->gMessageData->_first_free;

    // set the destination char (message) to the first free character
    message = globals()->gMessageData->_data.get() + *freeoffset;

    // the last char we're resting on gets turned into an end char, since this should be the last message
    *message = kMessageEndChar;
}

void StartLongMessage( short startResID, short endResID)

{
    longMessageType *tmessage;

    tmessage = globals()->gLongMessageData.get();

    if ( tmessage->currentResID != -1)
    {
        tmessage->startResID = startResID;
        tmessage->endResID = endResID;
        tmessage->currentResID = startResID - 1;
        AdvanceCurrentLongMessage();
    } else
    {
        tmessage->previousStartResID = tmessage->startResID;
        tmessage->previousEndResID = tmessage->endResID;
        tmessage->startResID = startResID;
        tmessage->endResID = endResID;
        tmessage->currentResID = startResID;
        tmessage->time = 0;
        tmessage->stage = kStartStage;
        tmessage->textHeight = 0;
        if ( tmessage->retroTextSpec.text.get() != nil) {
            tmessage->retroTextSpec.text.destroy();
        }
        tmessage->retroTextSpec.textLength = 0;
        tmessage->retroTextSpec.thisPosition = 0;
        tmessage->retroTextSpec.topBuffer = kMessageCharTopBuffer;
        tmessage->retroTextSpec.bottomBuffer = kMessageCharBottomBuffer;
        tmessage->charDelayCount = 0;
        tmessage->pictBounds.left = tmessage->pictBounds.right= 0;
        // tmessage->pictDelayCount;
        tmessage->pictCurrentLeft = 0;
        tmessage->pictCurrentTop = 0;
        tmessage->pictID = -1;
        WriteDebugLine("\pSTART MESSAGE");
    }
}

void StartStringMessage(unsigned char* string)

{
    longMessageType *tmessage;

    tmessage = globals()->gLongMessageData.get();

    tmessage->newStringMessage = true;
    if ( tmessage->currentResID != -1)
    {
        tmessage->startResID = kStringMessageID;
        tmessage->endResID = kStringMessageID;
        tmessage->currentResID = kStringMessageID;
        CopyAnyCharPString( tmessage->stringMessage, string);
        tmessage->stage = kStartStage;
    } else
    {
        tmessage->previousStartResID = tmessage->startResID;
        tmessage->previousEndResID = tmessage->endResID;
        tmessage->startResID = kStringMessageID;
        tmessage->endResID = kStringMessageID;
        tmessage->currentResID = kStringMessageID;
        tmessage->time = 0;
        tmessage->stage = kStartStage;
        tmessage->textHeight = 0;
        if (tmessage->retroTextSpec.text.get() != nil) {
            tmessage->retroTextSpec.text.destroy();
        }
        tmessage->retroTextSpec.textLength = 0;
        tmessage->retroTextSpec.thisPosition = 0;
        tmessage->retroTextSpec.topBuffer = kMessageCharTopBuffer;
        tmessage->retroTextSpec.bottomBuffer = kMessageCharBottomBuffer;
        tmessage->charDelayCount = 0;
        tmessage->pictBounds.left = tmessage->pictBounds.right= 0;
        // tmessage->pictDelayCount;
        tmessage->pictCurrentLeft = 0;
        tmessage->pictCurrentTop = 0;
        tmessage->pictID = -1;
        CopyAnyCharPString( tmessage->stringMessage, string);
        WriteDebugLine( "\pSTART MESSAGE");
    }
}

void ClipToCurrentLongMessage( void)

{
    longMessageType *tmessage;
    TypedHandle<unsigned char> textData;
    transColorType  *transColor;
    unsigned char* ac;
    long            count;

    tmessage = globals()->gLongMessageData.get();
    if (( tmessage->currentResID != tmessage->lastResID) || ( tmessage->newStringMessage))
    {

        if ( tmessage->lastResID >= 0)
        {
            CLIP_BOTTOM = globals()->gTrueClipBottom;
        }

        // draw in offscreen world
        if (( tmessage->currentResID >= 0) && ( tmessage->stage == kClipStage))
        {
            if ( tmessage->currentResID == kStringMessageID)
            {
                textData.create(tmessage->stringMessage[0]);
                if (textData.get() != nil) {
                    count = 1;
                    ac = *textData;
                    while ( count <= tmessage->stringMessage[0])
                    {
                        *ac = tmessage->stringMessage[count];
                        ac++;
                        count++;
                    }
                }
                tmessage->labelMessage = false;
            } else
            {
                textData.load_resource('TEXT', tmessage->currentResID);
                Replace_KeyCode_Strings_With_Actual_Key_Names( textData,
                    kKeyMapNameLongID, 0);
                if ( **textData == '#')
                {
                    tmessage->labelMessage = true;
                }
                else tmessage->labelMessage = false;

            }
            if (textData.get() != nil)
            {
//              tmessage->textHeight = GetInterfaceTextHeightFromWidth( (anyCharType *)*textData, GetHandleSize( textData),
//                                  kLarge, CLIP_RIGHT - CLIP_LEFT);
                mSetDirectFont( kLongMessageFontNum);
                if (tmessage->retroTextSpec.text.get() != nil) {
                    tmessage->retroTextSpec.text.destroy();
                }
                tmessage->retroTextSpec.text = textData;
                tmessage->retroTextSpec.textLength = tmessage->retroTextSpec.text.count();
                tmessage->textHeight = DetermineDirectTextHeightInWidth( &tmessage->retroTextSpec,
                (CLIP_RIGHT - CLIP_LEFT) - kHBufferTotal);
                tmessage->textHeight += kLongMessageVPadDouble;

                if ( tmessage->labelMessage == false)
                    CLIP_BOTTOM = globals()->gTrueClipBottom - tmessage->textHeight;
                else
                    CLIP_BOTTOM = globals()->gTrueClipBottom;

                tmessage->retroTextSpec.topBuffer = kMessageCharTopBuffer;
                tmessage->retroTextSpec.bottomBuffer = kMessageCharBottomBuffer;
                tmessage->retroTextSpec.thisPosition = 0;
                tmessage->retroTextSpec.lineCount = 0;
                tmessage->retroTextSpec.linePosition = 0;
                tmessage->retroTextSpec.xpos = CLIP_LEFT + kHBuffer;
                tmessage->retroTextSpec.ypos = CLIP_BOTTOM + mDirectFontAscent() + kLongMessageVPad + tmessage->retroTextSpec.topBuffer;
                tmessage->stage = kShowStage;
                tmessage->retroTextSpec.tabSize = 60;
                mGetTranslateColorShade( SKY_BLUE, VERY_LIGHT, tmessage->retroTextSpec.color, transColor);
                mGetTranslateColorShade( SKY_BLUE, DARKEST, tmessage->retroTextSpec.backColor, transColor);
                tmessage->retroTextSpec.nextColor = tmessage->retroTextSpec.color;
                tmessage->retroTextSpec.nextBackColor = tmessage->retroTextSpec.backColor;
                tmessage->retroTextSpec.originalColor = tmessage->retroTextSpec.color;
                tmessage->retroTextSpec.originalBackColor = tmessage->retroTextSpec.backColor;
            }
        } else
        {
            CLIP_BOTTOM = globals()->gTrueClipBottom;
            tmessage->stage = kClipStage;
        }
    }
}

void DrawCurrentLongMessage( long timePass)

{
    Rect            tRect, uRect;
    Rect        lRect, cRect;
    transColorType  *transColor;
    short           i;
    longMessageType *tmessage;
    unsigned char   color;

    tmessage = globals()->gLongMessageData.get();
    if (( tmessage->currentResID != tmessage->lastResID) ||
        ( tmessage->newStringMessage))
    {
        // we check scenario conditions here for ambrosia tutorial
        // but not during net game -- other players wouldn't care what message
        // we were looking at
        if ( !(globals()->gOptions & kOptionNetworkOn))
        {
            CheckScenarioConditions( 0);
        }

        if (tmessage->lastResID >= 0)
        {
            if ( tmessage->lastLabelMessage)
            {
                SetScreenLabelAge( tmessage->labelMessageID, 1);
            } else
            {
                DrawInOffWorld();
                SetLongRect( &lRect, CLIP_LEFT, globals()->gTrueClipBottom - tmessage->textHeight, CLIP_RIGHT,
                        globals()->gTrueClipBottom);
                cRect = lRect;
                DrawNateRect(gOffWorld, &cRect, 0, 0, 0xff);
                LongRectToRect( &lRect, &tRect);
                ChunkCopyPixMapToScreenPixMap(gOffWorld, &tRect, gActiveWorld);
                NormalizeColors();
                DrawInRealWorld();
    //          CLIP_BOTTOM = globals()->gTrueClipBottom;
                NormalizeColors();
            }
        }

        // draw in offscreen world
        if (( tmessage->currentResID >= 0) && ( tmessage->stage == kShowStage))
        {
            if (tmessage->retroTextSpec.text.get() != nil) {
                if ( !tmessage->labelMessage)
                {
                    DrawInOffWorld();
                    SetLongRect( &lRect, CLIP_LEFT, CLIP_BOTTOM, CLIP_RIGHT,
                            globals()->gTrueClipBottom);
                    mGetTranslateColorShade( SKY_BLUE, DARKEST, color, transColor);
                    cRect = lRect;
                    DrawNateRect(gOffWorld, &cRect, 0, 0, color);
                    LongRectToRect( &lRect, &tRect);
                    mGetTranslateColorShade( SKY_BLUE, VERY_LIGHT, color, transColor);
    //              DrawDirectTextInRect( (anyCharType *)*tmessage->retroTextSpec.text, tmessage->retroTextSpec.textLength,
    //                      &lRect, *offPixBase, 0, 0, 0);
                    DrawNateLine(gOffWorld, &cRect, cRect.left, cRect.top, cRect.right - 1,
                        cRect.top, 0, 0, color);
                    DrawNateLine(gOffWorld, &cRect, cRect.left, cRect.bottom - 1, cRect.right - 1,
                        cRect.bottom - 1, 0, 0, color);
                    ChunkCopyPixMapToScreenPixMap(gOffWorld, &tRect, gActiveWorld);
                    NormalizeColors();
                    DrawInRealWorld();
                    NormalizeColors();
                } else
                {
                    SetScreenLabelAge( tmessage->labelMessageID, 0);

                    MessageLabel_Set_Special( tmessage->labelMessageID,
                        tmessage->retroTextSpec.text);
                }
            }
        } else if ( !tmessage->labelMessage)
        {
            DrawInOffWorld();
            SetLongRect( &lRect, CLIP_LEFT, globals()->gTrueClipBottom - tmessage->textHeight, CLIP_RIGHT,
                    globals()->gTrueClipBottom);
            cRect = lRect;
            DrawNateRect(gOffWorld, &cRect, 0, 0, 0xff);
            LongRectToRect( &lRect, &tRect);
            ChunkCopyPixMapToScreenPixMap(gOffWorld, &tRect, gActiveWorld);
            NormalizeColors();
            DrawInRealWorld();
//          CLIP_BOTTOM = globals()->gTrueClipBottom;
            NormalizeColors();
        }
        if (( tmessage->stage == kShowStage) || (  tmessage->currentResID < 0))
        {
            tmessage->lastResID = tmessage->currentResID;
            tmessage->lastLabelMessage = tmessage->labelMessage;
            tmessage->newStringMessage = false;
        }
    } else
    {
        if ((tmessage->labelMessage) && (tmessage->retroTextSpec.text.get() != nil)) {
            tmessage->retroTextSpec.text.destroy();
        } else if ((tmessage->currentResID >= 0) && (tmessage->retroTextSpec.text.get() != nil) &&
            ( tmessage->retroTextSpec.thisPosition < tmessage->retroTextSpec.textLength) && ( tmessage->stage == kShowStage))
        {
            tmessage->charDelayCount += timePass;
            if ( tmessage->charDelayCount > 0)
            {
                mSetDirectFont( kLongMessageFontNum);
                SetLongRect( &lRect, CLIP_LEFT, CLIP_BOTTOM, CLIP_RIGHT,
                        globals()->gTrueClipBottom);
                PlayVolumeSound(  kTeletype, kMediumLowVolume, kShortPersistence, kLowPrioritySound);
                while ( tmessage->charDelayCount > 0)
                {
                    i = 3;

                    if ((tmessage->retroTextSpec.text.get() != nil) &&
                        ( tmessage->retroTextSpec.thisPosition < tmessage->retroTextSpec.textLength))
                    {

                        tRect.left = tmessage->retroTextSpec.xpos;
                        tRect.top = tmessage->retroTextSpec.ypos -
                            (mDirectFontAscent()  + tmessage->retroTextSpec.topBuffer);
                        tRect.right = tRect.left + gDirectText->logicalWidth;
                        tRect.bottom = tRect.top + mDirectFontHeight() +
                            tmessage->retroTextSpec.topBuffer +
                            tmessage->retroTextSpec.bottomBuffer;

                        lRect.left += kHBuffer;
                        lRect.right -= kHBuffer;

                        DrawRetroTextCharInRect( &(tmessage->retroTextSpec),
                            3, &lRect, &lRect, gOffWorld, 0, 0);
//                           *thePixMapHandle, gNatePortLeft, gNatePortTop);

                        lRect.left -= kHBuffer;
                        lRect.right += kHBuffer;

                        uRect.left = tmessage->retroTextSpec.xpos;
                        uRect.top = tmessage->retroTextSpec.ypos -
                            (mDirectFontAscent()  + tmessage->retroTextSpec.topBuffer);
                        uRect.right = uRect.left + gDirectText->logicalWidth;
                        uRect.bottom = uRect.top + mDirectFontHeight() +
                            tmessage->retroTextSpec.topBuffer +
                                tmessage->retroTextSpec.bottomBuffer;
                        if ( uRect.left <= tRect.left)
                        {
                            uRect.right = lRect.right;
                            uRect.left = lRect.left;
                        }
                        BiggestRect( &tRect, &uRect);
                        ChunkCopyPixMapToScreenPixMap(gOffWorld, &tRect,
                            gActiveWorld);
                        if ( tmessage->retroTextSpec.thisPosition >
                            tmessage->retroTextSpec.textLength)
                        {
                            tmessage->retroTextSpec.text.destroy();
                        }
                    }
                    tmessage->charDelayCount -= 3;
                }
            }
        }
    }
}

void EndLongMessage( void)

{
    longMessageType *tmessage;

    tmessage = globals()->gLongMessageData.get();
    tmessage->previousStartResID = tmessage->startResID;
    tmessage->previousEndResID = tmessage->endResID;
    tmessage->startResID = -1;
    tmessage->endResID = -1;
    tmessage->currentResID = -1;
    tmessage->stage = kStartStage;
    if (tmessage->retroTextSpec.text.get() != nil) {
        tmessage->retroTextSpec.text.destroy();
    }
    CopyAnyCharPString( tmessage->lastStringMessage, tmessage->stringMessage);
}

void AdvanceCurrentLongMessage( void)
{
    longMessageType *tmessage;

    tmessage = globals()->gLongMessageData.get();
    if ( tmessage->currentResID != -1)
    {
        if ( tmessage->currentResID < tmessage->endResID)
        {
            tmessage->currentResID++;
            tmessage->stage = kStartStage;
        }
        else
        {
            EndLongMessage();
        }
    }
}

void PreviousCurrentLongMessage( void)
{
    longMessageType *tmessage;

    tmessage = globals()->gLongMessageData.get();
    if ( tmessage->currentResID != -1)
    {
        if ( tmessage->currentResID > tmessage->startResID)
        {
            tmessage->currentResID--;
            tmessage->stage = kStartStage;
        }
        else
        {
        }
    }
}

void ReplayLastLongMessage( void)
{
    longMessageType *tmessage;

    tmessage = globals()->gLongMessageData.get();
    if (( tmessage->previousStartResID >= 0) && ( tmessage->currentResID < 0))
    {
        CopyAnyCharPString( tmessage->stringMessage, tmessage->lastStringMessage);
        StartLongMessage( tmessage->previousStartResID, tmessage->previousEndResID);
    }
}

// WARNING: RELIES ON kMessageNullCharacter (SPACE CHARACTER #32) >> NOT WORLD-READY <<

void DrawMessageScreen( long byUnits)

{
#ifdef kUseMessage
    Str255          tString;
    unsigned char   *anyChar, *dChar, *tLen;
    int32_t* firstoffset;
    int32_t offset;

    // increase the amount of time current message has been shown
    globals()->gMessageTimeCount += byUnits;

    // if it's been shown for too long, then get the next message
    if ( globals()->gMessageTimeCount > kMessageDisplayTime)
    {
        globals()->gMessageTimeCount = 0;
        // get the offset to the first current char
        firstoffset = &globals()->gMessageData->_first_char;
        anyChar = globals()->gMessageData->_data.get() + *firstoffset;
        if ( *anyChar != kMessageEndChar)
        {
            offset = *firstoffset;

            do
            {
                // increase dest char
                anyChar++;

                // increase offset copy
                offset++;

                // if the offset == the length of our data (in chars) then wrap around
                if (implicit_cast<uint32_t>(offset) >= kMaxMessageLength) {
                    // reset offset to first char data (after the two longs of offsets)
                    offset = kAnyCharOffsetStart;

                    // reset the destination char to the first free char
                    anyChar = globals()->gMessageData->_data.get() + offset;
                }
            } while (( *anyChar != kMessageSeparateChar) && ( *anyChar != kMessageEndChar));
            *firstoffset = offset;
        }
    }

    mSetDirectFont( kTacticalFontNum);

    // get the offset to the first current char
    firstoffset = &globals()->gMessageData->_first_char;
    anyChar = globals()->gMessageData->_data.get() + *firstoffset;
    if ( *anyChar != kMessageEndChar)
    {
        tLen = dChar = tString;
        *tLen = 0;
        dChar++;
        offset = *firstoffset;

        do
        {
            // increase dest char
            anyChar++;

            // increase offset copy
            offset++;

            // if the offset == the length of our data (in chars) then wrap around
            if (offset >= kMaxMessageLength)
            {
                // reset offset to first char data (after the two longs of offsets)
                offset = kAnyCharOffsetStart;

                // reset the destination char to the first free char
                anyChar = globals()->gMessageData->_data.get() + offset;
            }
            *dChar = *anyChar;
            dChar++;
            *tLen += 1;
        } while (( *anyChar != kMessageSeparateChar) && ( *anyChar != kMessageEndChar));
        *tLen -= 1;

        if ( globals()->gMessageTimeCount < kRaiseTime)
        {
            SetScreenLabelPosition( globals()->gMessageLabelNum, kMessageScreenLeft,
                    CLIP_BOTTOM - globals()->gMessageTimeCount);
        } else if ( globals()->gMessageTimeCount > kLowerTime)
        {
            SetScreenLabelPosition( globals()->gMessageLabelNum, kMessageScreenLeft,
                    CLIP_BOTTOM - ( kMessageDisplayTime - globals()->gMessageTimeCount));
        }

        SetScreenLabelString( globals()->gMessageLabelNum, tString);
    } else
    {
        SetScreenLabelString( globals()->gMessageLabelNum, nil);
        globals()->gMessageTimeCount = 0;
    }

#endif
}


// SetStatusString: pass nil to set to 0

void SetStatusString(const unsigned char *s, bool drawNow, unsigned char color)

{
#pragma unused( drawNow)
/*
    if (( s != nil) && ( *s != 0))
    {
        CopyAnyCharPString( (anyCharType *)*globals()->gStatusString, s);
    } else
    {
        **globals()->gStatusString = 0;
    }

    if ( drawNow) UpdateStatusString();
*/
    SetScreenLabelColor( globals()->gStatusLabelNum, color);
    SetScreenLabelString( globals()->gStatusLabelNum, s);
    SetScreenLabelAge( globals()->gStatusLabelNum, kStatusLabelAge);

}

void UpdateStatusString( void)

{

/*
    mSetDirectFont( kMessageFontNum)

    offPixBase = GetGWorldPixMap( gOffWorld);
    DrawInOffWorld();
    SetLongRect( &lRect, kDestinationLeft, kDestinationTop, kDestinationRight,
            kDestinationBottom);
    color = GetTranslateColorShade( kDestinationColor, VERY_DARK);
    cRect = lRect;
    DrawNateRect( *offPixBase, &cRect, 0, 0, color);
    MoveTo( kDestinationLeft, kDestinationVCenter - gDirectText->height + gDirectText->ascent * 2);
    if ( **globals()->gStatusString != 0)
        DrawDirectTextStringClippedx2(  (anyCharType *)*globals()->gStatusString,
                                    GetTranslateColorShade( kDestinationColor, VERY_LIGHT),
                                    *offPixBase, &lRect, 0, 0);
    LongRectToRect( &lRect, &tRect);
    ChunkCopyPixMapToScreenPixMap( *offPixBase, &tRect, *thePixMapHandle);
    NormalizeColors();
    DrawInRealWorld();
    NormalizeColors();
*/
}

long DetermineDirectTextHeightInWidth( retroTextSpecType *retroTextSpec, long inWidth)

{
    long            charNum = 0, height = mDirectFontHeight(), x = 0, oldx = 0, oldCharNum, wordLen,
                    *lineLengthList = retroTextSpec->lineLength;
    unsigned char   *widthPtr, charWidth, wrapState; // 0 = none, 1 = once, 2 = more than once
    unsigned char   *thisChar = *retroTextSpec->text;

    *lineLengthList = 0;
    retroTextSpec->autoWidth = 0;
    retroTextSpec->lineNumber = 1;
    while ( charNum < retroTextSpec->textLength)
    {
        if ( *thisChar == kReturnChar)
        {
            if ( x > retroTextSpec->autoWidth) retroTextSpec->autoWidth = x;
            height += mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
            x = 0;
            thisChar++;
            charNum++;
            (*lineLengthList)++;
            lineLengthList++;
            *lineLengthList = 0;
            retroTextSpec->lineNumber++;
        } else if ( *thisChar == ' ')
        {
            mDirectCharWidth( charWidth, ' ', widthPtr);
            do
            {
                x += charWidth;
                thisChar++;
                charNum++;
                (*lineLengthList)++;
            } while (( *thisChar == ' ')  && ( charNum < retroTextSpec->textLength));
        } else if ( *thisChar == kCodeChar)
        {
            thisChar++;
            charNum++;
            (*lineLengthList)++;
            switch( *thisChar)
            {
                case kCodeTabChar:
                    wordLen = 0;
                    oldx = 0;
                    while ( oldx <= x)
                    {
                        oldx += retroTextSpec->tabSize;
                        wordLen++;
                    }
                    x = 0 + retroTextSpec->tabSize * wordLen;
                    break;

                case kCodeForeColorChar:
                case kCodeBackColorChar:
                    thisChar++;
                    charNum++;
                    (*lineLengthList)++;
                    thisChar++;
                    charNum++;
                    (*lineLengthList)++;
                    break;

                case kCodeChar:
                    mDirectCharWidth( charWidth, *thisChar, widthPtr);
                    x += charWidth;
                    wordLen++;
                    break;
            }
            thisChar++;
            charNum++;
            (*lineLengthList)++;
        } else
        {
            oldx = x;
            oldCharNum = charNum;
            wordLen = 0;
            wrapState = 0;
            do
            {
                mDirectCharWidth( charWidth, *thisChar, widthPtr);
                x += charWidth;
                wordLen++;
                if ( x >= (inWidth - gDirectText->logicalWidth))
                {
                    if ( !wrapState)
                    {
                        wrapState = 1;
                        if ( oldx > retroTextSpec->autoWidth) retroTextSpec->autoWidth = oldx;
                        x = x - oldx;
                        oldx = 0;
                        height += mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                    } else
                    {
                        wrapState = 2;
                        wordLen--;
                    }
                }
                thisChar++;
                charNum++;
            } while (( *thisChar != ' ') && ( *thisChar != kReturnChar) && ( wrapState < 2) &&
                ( *thisChar != kCodeChar) && ( charNum < retroTextSpec->textLength));
            if ( wrapState)
            {
                if ( x > retroTextSpec->autoWidth) retroTextSpec->autoWidth = x;
                lineLengthList++;
                *lineLengthList = 0;
                retroTextSpec->lineNumber++;
            }
            *lineLengthList += wordLen;
        }
    }
    if ( x > retroTextSpec->autoWidth) retroTextSpec->autoWidth = x;
    retroTextSpec->autoWidth += 1;
    retroTextSpec->autoHeight = height;
    return ( height);
}

void DrawDirectTextInRect( retroTextSpecType *retroTextSpec, Rect *bounds, Rect *clipRect, PixMap *destMap,
                        long portLeft, long portTop)
{
    long            charNum = 0, y = bounds->top + mDirectFontAscent() + retroTextSpec->topBuffer, x = bounds->left,
                    oldx = 0, oldCharNum, wordLen;
    unsigned char   *widthPtr, charWidth, wrapState, // 0 = none, 1 = once, 2 = more than once
                    tempColor;
    unsigned char   *thisChar = *retroTextSpec->text, *thisWordChar, thisWord[255];
    Rect        backRect, lineRect;
    unsigned char   calcColor, calcShade;
    transColorType  *transColor;

    while ( charNum < retroTextSpec->textLength)
    {
        if ( *thisChar == kReturnChar)
        {
            lineRect.left = x;
            lineRect.right = bounds->right;
            lineRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
            lineRect.bottom = lineRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
            DrawNateRectClipped( destMap, &lineRect, clipRect, (portLeft << 2),
                portTop, retroTextSpec->backColor);

            y += mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
            x = bounds->left;
            thisChar++;
            charNum++;
        } else if ( *thisChar == ' ')
        {
            backRect.left = x;
            backRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
            backRect.bottom = backRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
            mDirectCharWidth( charWidth, ' ', widthPtr);
            do
            {
                x += charWidth;
                thisChar++;
                charNum++;
            } while (( *thisChar == ' ')  && ( charNum < retroTextSpec->textLength));
            backRect.right = x;
            DrawNateRectClipped( destMap, &backRect, clipRect, (portLeft << 2),
                portTop, retroTextSpec->backColor);
        } else if ( *thisChar == kCodeChar)
        {
            thisChar++;
            charNum++;
            switch( *thisChar)
            {
                case kCodeTabChar:
                    wordLen = 0;
                    oldx = bounds->left;
                    backRect.left = x;
                    backRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
                    backRect.bottom = backRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                    while ( oldx <= x)
                    {
                        oldx += retroTextSpec->tabSize;
                        wordLen++;
                    }
                    x = bounds->left + retroTextSpec->tabSize * wordLen;
                    backRect.right = x;
                    DrawNateRectClipped( destMap, &backRect, clipRect, (portLeft << 2),
                        portTop, retroTextSpec->backColor);
                    break;

                case kCodeChar:
                    oldx = backRect.left = x;
                    backRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
                    backRect.bottom = backRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                    mDirectCharWidth( charWidth, *thisChar, widthPtr);
                    x += charWidth;
                    DrawNateRectClipped( destMap, &backRect, clipRect, (portLeft << 2),
                        portTop, retroTextSpec->backColor);
                    thisWord[0] = 1;
                    thisWord[1] = kCodeChar;
                    backRect.right = x;
                    MoveTo( oldx, y);
                    DrawDirectTextStringClipped( thisWord, retroTextSpec->color, destMap, clipRect, portLeft,
                            portTop);
                    break;

                case kCodeInvertChar:
                    tempColor = retroTextSpec->color;
                    retroTextSpec->color = retroTextSpec->backColor;
                    retroTextSpec->backColor = tempColor;
                    break;

                case kCodeForeColorChar:
                    thisChar++;
                    charNum++;
                    (retroTextSpec->thisPosition)++;
                    (retroTextSpec->linePosition)++;
                    calcColor = mHexDigitValue(*thisChar);
                    thisChar++;
                    charNum++;
                    (retroTextSpec->thisPosition)++;
                    (retroTextSpec->linePosition)++;
                    calcShade = mHexDigitValue(*thisChar);
                    mGetTranslateColorShade( calcColor, calcShade, retroTextSpec->color, transColor);
                    break;

                case kCodeBackColorChar:
                    thisChar++;
                    charNum++;
                    calcColor = mHexDigitValue(*thisChar);
                    thisChar++;
                    charNum++;
                    calcShade = mHexDigitValue(*thisChar);
                    if (( calcColor == 0) && (calcShade == 0)) retroTextSpec->backColor = 0xff;
                    else
                    {
                        mGetTranslateColorShade( calcColor, calcShade, retroTextSpec->backColor, transColor);
                    }
                    break;

                case kCodeRevertChar:
                    retroTextSpec->color = retroTextSpec->originalColor;
                    retroTextSpec->backColor = retroTextSpec->originalBackColor;
                    break;
            }
            thisChar++;
            charNum++;
        } else
        {
            backRect.left = x;
            backRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
            backRect.bottom = backRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
            oldx = x;
            oldCharNum = charNum;
            wordLen = 0;
            thisWordChar = thisWord;
            thisWordChar++;
            wrapState = 0;
            do
            {
                mDirectCharWidth( charWidth, *thisChar, widthPtr);
                x += charWidth;
                wordLen++;
                *thisWordChar = *thisChar;
                thisWordChar++;
                if ( x >= (bounds->right /*- gDirectText->logicalWidth*/))
                {
                    if ( !wrapState)
                    {
                        lineRect.left = oldx;
                        lineRect.right = bounds->right;
                        lineRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
                        lineRect.bottom = lineRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                        DrawNateRectClipped( destMap, &lineRect, clipRect, (portLeft << 2),
                            portTop, retroTextSpec->backColor);

                        wrapState = 1;
                        x = bounds->left + (x - oldx);
                        oldx = bounds->left;
                        y += mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                        backRect.left = x;
                        backRect.top = y - (mDirectFontAscent() + retroTextSpec->topBuffer);
                        backRect.bottom = backRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                    } else
                    {
                        wrapState = 2;
                        wordLen--;
                    }
                }
                thisChar++;
                charNum++;
            } while (( *thisChar != ' ') && ( *thisChar != kReturnChar) &&
                ( *thisChar != kCodeChar) && ( wrapState < 2) &&
                ( charNum < retroTextSpec->textLength));
            thisWord[0] = wordLen;
            backRect.right = x;
            DrawNateRectClipped( destMap, &backRect, clipRect, (portLeft << 2),
                portTop, retroTextSpec->backColor);
            MoveTo( oldx, y);
            DrawDirectTextStringClipped( thisWord, retroTextSpec->color, destMap, clipRect, portLeft,
                    portTop);
        }
    }
}

void DrawRetroTextCharInRect( retroTextSpecType *retroTextSpec, long charsToDo,
    Rect *bounds, Rect *clipRect, PixMap *destMap, long portLeft, long portTop)
{
    unsigned char   *thisChar = *(retroTextSpec->text), thisWord[kMaxRetroSize], charWidth, *widthPtr;
    Rect        cursorRect, lineRect, tlRect;
    long            oldx, wordLen, *lineLength = &(retroTextSpec->lineLength[retroTextSpec->lineCount]);
    unsigned char   tempColor, calcColor, calcShade;
    transColorType  *transColor;
    bool         drawCursor = ( charsToDo > 0);

    cursorRect.left = retroTextSpec->xpos;
    cursorRect.top = retroTextSpec->ypos -
        (mDirectFontAscent()  + retroTextSpec->topBuffer);
    cursorRect.right = cursorRect.left + gDirectText->logicalWidth;
    cursorRect.bottom = cursorRect.top + mDirectFontHeight() +
        retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
    mCopyAnyRect( tlRect, cursorRect);
    mClipAnyRect( tlRect, *bounds);
    if ( retroTextSpec->originalBackColor != WHITE)
        DrawNateRectClipped( destMap, &tlRect, clipRect, (portLeft << 2),
            portTop, retroTextSpec->originalBackColor);

    if ( charsToDo <= 0) charsToDo = retroTextSpec->lineLength[retroTextSpec->lineCount];

    while (( charsToDo > 0) && ( retroTextSpec->thisPosition <
        retroTextSpec->textLength))
    {
        thisChar = *(retroTextSpec->text) + retroTextSpec->thisPosition;
        if ( *thisChar == kCodeChar)
        {
            thisChar++;
            charsToDo--;
            (retroTextSpec->thisPosition)++;
            (retroTextSpec->linePosition)++;
            switch( *thisChar)
            {
                case kCodeTabChar:
                    wordLen = 0;
                    oldx = bounds->left;
                    cursorRect.left = retroTextSpec->xpos;
                    while ( oldx <= retroTextSpec->xpos)
                    {
                        oldx += retroTextSpec->tabSize;
                        wordLen++;
                    }
                    retroTextSpec->xpos = bounds->left + retroTextSpec->tabSize *
                        wordLen;
                    cursorRect.right = retroTextSpec->xpos;
                    mCopyAnyRect( tlRect, cursorRect);
                    mClipAnyRect( tlRect, *bounds);
                    if ( retroTextSpec->backColor != WHITE)
                        DrawNateRectClipped( destMap, &tlRect, clipRect, (portLeft << 2), portTop,
                            retroTextSpec->backColor);
                    break;

                case kCodeChar:
                    oldx = cursorRect.left = retroTextSpec->xpos;
                    cursorRect.top = retroTextSpec->ypos - (mDirectFontAscent() + retroTextSpec->topBuffer);
                    cursorRect.bottom = cursorRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
                    mDirectCharWidth( charWidth, *thisChar, widthPtr);
                    retroTextSpec->xpos += charWidth;
                    mCopyAnyRect( tlRect, cursorRect);
                    mClipAnyRect( tlRect, *bounds);
                    if ( retroTextSpec->backColor != WHITE)
                        DrawNateRectClipped( destMap, &tlRect, clipRect, (portLeft << 2),
                            portTop, retroTextSpec->backColor);
                    thisWord[0] = 1;
                    thisWord[1] = '\\';
                    cursorRect.right = retroTextSpec->xpos;
                    MoveTo( oldx, retroTextSpec->ypos);
                    DrawDirectTextStringClipped( thisWord,
                            (retroTextSpec->color==WHITE)?(BLACK):
                                (retroTextSpec->color),
                            destMap, clipRect, portLeft,
                            portTop);
                    break;

                case kCodeInvertChar:
                    tempColor = retroTextSpec->color;
                    retroTextSpec->nextColor = retroTextSpec->backColor;
                    retroTextSpec->nextBackColor = tempColor;
                    break;

                case kCodeForeColorChar:
                    thisChar++;
                    charsToDo--;
                    (retroTextSpec->thisPosition)++;
                    (retroTextSpec->linePosition)++;
                    calcColor = mHexDigitValue(*thisChar);
                    thisChar++;
                    charsToDo--;
                    (retroTextSpec->thisPosition)++;
                    (retroTextSpec->linePosition)++;
                    calcShade = mHexDigitValue(*thisChar);
                    mGetTranslateColorShade( calcColor, calcShade, retroTextSpec->nextColor, transColor);
                    break;

                case kCodeBackColorChar:
                    thisChar++;
                    charsToDo--;
                    (retroTextSpec->thisPosition)++;
                    (retroTextSpec->linePosition)++;
                    calcColor = mHexDigitValue(*thisChar);
                    thisChar++;
                    charsToDo--;
                    (retroTextSpec->thisPosition)++;
                    (retroTextSpec->linePosition)++;
                    calcShade = mHexDigitValue(*thisChar);
                    if (( calcColor) && (calcShade == 0))
                    {
                        retroTextSpec->nextBackColor = 0xff;
                    } else
                    {
                        mGetTranslateColorShade( calcColor, calcShade, retroTextSpec->nextBackColor, transColor);
                    }
                    break;

                case kCodeRevertChar:
                    retroTextSpec->nextColor = retroTextSpec->originalColor;
                    retroTextSpec->nextBackColor = retroTextSpec->originalBackColor;
                    break;
            }
            thisChar++;
            charsToDo--;
            (retroTextSpec->thisPosition)++;
            (retroTextSpec->linePosition)++;
        } else if ( *thisChar == kReturnChar)
        {
            thisChar++;
            (retroTextSpec->thisPosition)++;
            (retroTextSpec->linePosition)++;
            charsToDo--;
        } else
        {
            thisWord[0] = 1;
            if ( *thisChar == '_')
                thisWord[1] = ' ';
            else
                thisWord[1] = *thisChar;
//          thisWord[1] = ' ';

            retroTextSpec->color = retroTextSpec->nextColor;
            retroTextSpec->backColor = retroTextSpec->nextBackColor;
            cursorRect.left = retroTextSpec->xpos;
            MoveTo( retroTextSpec->xpos, retroTextSpec->ypos);
            mDirectCharWidth( charWidth, *thisChar, widthPtr);
            retroTextSpec->xpos += charWidth;
            cursorRect.right = retroTextSpec->xpos;
            mCopyAnyRect( tlRect, cursorRect);
            mClipAnyRect( tlRect, *bounds);
            if ( retroTextSpec->backColor != WHITE)
                DrawNateRectClipped( destMap, &tlRect, clipRect, (portLeft << 2), portTop,
                    retroTextSpec->backColor);
            DrawDirectTextStringClipped( thisWord,
                (retroTextSpec->color==WHITE)?(BLACK):(retroTextSpec->color),
                destMap, clipRect, portLeft, portTop);
            (retroTextSpec->thisPosition)++;
            (retroTextSpec->linePosition)++;
            charsToDo--;
        }
        if ( retroTextSpec->linePosition >= *lineLength)
        {
            lineRect.left = retroTextSpec->xpos;
            lineRect.right = bounds->right;
            lineRect.top = cursorRect.top;
            lineRect.bottom = cursorRect.bottom;
            mCopyAnyRect( tlRect, lineRect);
            mClipAnyRect( tlRect, *bounds);
            if ( retroTextSpec->backColor != WHITE)
                DrawNateRectClipped( destMap, &tlRect, clipRect, (portLeft << 2), portTop,
                    retroTextSpec->backColor);

            retroTextSpec->linePosition = 0;
            retroTextSpec->ypos += mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
            retroTextSpec->xpos = bounds->left;
            (retroTextSpec->lineCount)++;
            lineLength++;
            cursorRect.top = retroTextSpec->ypos - (mDirectFontAscent() + retroTextSpec->topBuffer);
            cursorRect.bottom = cursorRect.top + mDirectFontHeight() + retroTextSpec->topBuffer + retroTextSpec->bottomBuffer;
        } else
        {
        }

    }
    if ( retroTextSpec->thisPosition < retroTextSpec->textLength)
    {
        cursorRect.left = retroTextSpec->xpos;
        cursorRect.right = cursorRect.left + gDirectText->logicalWidth;
        if ( drawCursor)
        {
            mCopyAnyRect( tlRect, cursorRect);
            mClipAnyRect( tlRect, *bounds);
            if ( retroTextSpec->backColor != WHITE)
                DrawNateRectClipped( destMap, &tlRect, clipRect, (portLeft << 2), portTop,
                    retroTextSpec->originalColor);
        }
    }
}

//
// MessageLabel_Set_Special
//  for ambrosia emergency tutorial; Sets screen label given specially formatted
//  text. Text must have its own line breaks so label fits on screen.
//
//  First few chars of text must be in this format:
//
//  #tnnn...#
//
//  Where '#' is literal;
//  t = one of three characters: 'L' for left, 'R' for right, and 'O' for object
//  nnn... are digits specifying value (distance from top, or initial object #)
//
void MessageLabel_Set_Special(short id, TypedHandle<unsigned char> text) {
    unsigned char    whichType, *c;
    long    value = 0, charNum = 0, textLength, safetyCount;
    Str255  s;
    Point   attachPoint;
    bool hintLine = false;

    s[0] = 0;
    if (text.get() == nil) {
        return;
    }
    textLength = text.count();
    c = *text;

    // if not legal, bail
    if ( *c != '#') return;

    c++;
    charNum++;

    whichType = *c;
    c++;
    charNum++;
    safetyCount = 0;
    while (( *c != '#') && ( charNum < textLength) && ( safetyCount < 10)) // arbitrary safety net
    {
        value *= 10;
        value += *c - '0';
        c++;
        charNum++;
        safetyCount++;
    }

    c++;
    charNum++;
    if ( *c == '#') // also a hint line attached
    {
        hintLine = true;
        c++;
        charNum++;
        // h coord
        safetyCount = 0;
        while (( *c != ',') && ( charNum < textLength) && ( safetyCount < 10)) // arbitrary safety net
        {
            attachPoint.h *= 10;
            attachPoint.h += *c - '0';
            c++;
            charNum++;
            safetyCount++;
        }

        c++;
        charNum++;

        safetyCount = 0;
        while (( *c != '#') && ( charNum < textLength) && ( safetyCount < 10)) // arbitrary safety net
        {
            attachPoint.v *= 10;
            attachPoint.v += *c - '0';
            c++;
            charNum++;
            safetyCount++;
        }
        attachPoint.v += globals()->gInstrumentTop;
        if ( attachPoint.h >= (kSmallScreenWidth - kRightPanelWidth))
        {
            attachPoint.h = (attachPoint.h - (kSmallScreenWidth - kRightPanelWidth)) +
                globals()->gRightPanelLeftEdge;
        }
        c++;
        charNum++;
    }

    while (( charNum < textLength) && ( s[0] < 255))
    {
        s[0] += 1;
        s[s[0]] = *c;
        c++;
        charNum++;
    }
    SetScreenLabelString( id, s);
    SetScreenLabelKeepOnScreenAnyway( id, true);
    switch( whichType)
    {
        case 'R':
            SetScreenLabelOffset( id, 0, 0);

            SetScreenLabelPosition( id, globals()->gRightPanelLeftEdge -
                (GetScreenLabelWidth( id)+10), globals()->gInstrumentTop +
                value);
            break;

        case 'L':
            SetScreenLabelOffset( id, 0, 0);

            SetScreenLabelPosition( id, 138, globals()->gInstrumentTop +
                value);
            break;

        case 'O':
            {
                spaceObjectType         *o;
                scenarioInitialType     *initial;

                mGetRealObjectFromInitial( o, initial, value);

                SetScreenLabelOffset( id, -(GetScreenLabelWidth( id)/2), 64);

                SetScreenLabelObject( id, o);

                hintLine = true;
            }
            break;

    }
    attachPoint.v -= 2;
    SetScreenLabelAttachedHintLine( id, hintLine, attachPoint);
}

}  // namespace antares
