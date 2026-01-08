//-----------------------------------------------------------------------------
//
//  TITLE : MDK type definition
//	$Workfile: mdk_type.h $ 
//	$Revision: 1 $ 
//	$JustDate: 09/04/17 $ 
//
//	Copyright 2008 - 2013 OLYMPUS CORPORATION All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef	__MDK_TYPE_H__
#define	__MDK_TYPE_H__

#include <string>
using namespace std;

//=============================================================================
//	basic type

#ifdef	__WINDOWS__
typedef __int64		LONGLONG;
#else
typedef long long	LONGLONG;
#endif	// __WINDOWS__


//-----------------------------------------------------------------------------
//	constants
#define	MDK_MAX_AXIS		2

//=============================================================================
//	enum

//-----------------------------------------------------------------------------
//	unit list
typedef	enum tag_eDeviceType {
	e_DTUnknown				= 0,			// unknown unit

	// motor unit
	e_DTNosepiece			= 20000,		// nose piece
	e_DTFilterWheel			= 20002,		// filter wheel
	e_DTLamp				= 20003,		// lamp
	e_DTApertureStop		= 20004,		// AS
	e_DTShutter				= 20005,		// shutter
	e_DTMagnificationDevice = 20006,
	e_DTMagnificationChanger= 20007,		// 
	e_DTTopLens				= 20008,		// top lens
	e_DTLightPathSwitch		= 20009,		// light path switcher
	e_DTZoom				= 20010,		// zoom

	e_DTStageInsert			= 20050,

	e_DTSlideLoader			= 30000,

	e_DTManualControl		= 40000,

	e_DTFocus				= 25000,		// focus
	e_DTCondenser			= 25001,		// condenser
	e_DTMirror				= 25002,		// mirror
	e_DTHandSwitch			= 25003,		// hand switch
	e_DTAberLens			= 25004,		// aber lens
	e_DTFieldStop			= 25005,		// FS
	e_DTCorrectionRing		= 25007,		// correction ring

	e_DTStage				= 20051,		// stage
	e_DTSample				= 20052,		// sample, attached to stage ??

	e_DTMicroscope			= 60000,		// microscope system
	e_DTTiny				= 61000			// tiny board 

}	eDeviceType;


#endif	//	__MDK_TYPE_H__

