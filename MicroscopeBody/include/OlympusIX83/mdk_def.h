//-----------------------------------------------------------------------------
//
//  TITLE : MDK macro definition
//	$Workfile: mdk_def.h $ 
//	$Revision: 1 $ 
//	$JustDate: 09/04/17 $ 
//
//	Copyright 2008 - 2013 OLYMPUS CORPORATION All Rights Reserved.
//
//-----------------------------------------------------------------------------

#ifndef	__MDK_DEF_H__
#define	__MDK_DEF_H__

//-----------------------------------------------------------------------------
//	OS
#define		__WINDOWS__

//-----------------------------------------------------------------------------
//	in/out
#define		IN
#define		OUT
#define		BOTH

//-----------------------------------------------------------------------------
//	DLL
#define DLL_IMPORT __declspec(dllimport)
#define DLL_EXPORT __declspec(dllexport)

//-----------------------------------------------------------------------------
//	constant
#define	MAX_STRING				512			// max string size
#define	MAX_MDK_UNITS			100			// max units in microscope
#define	MAX_MPOWER_TIMEOUT		4000		// timeout constant

#define	MAX_FIRM_CMD_SEQUENCE	99			// command sequence number range.
#define	MIN_FIRM_CMD_SEQUENCE	0	

#define	MDK_FIRM_CMD_MAINTENACE	30000		// maintenance command no...
#define	MDK_FIRM_CMD_TINY		60000		// low level command no for Tiny board. (obsolete)

#define	MDK_INVALID_ITEM		0xffffffff	// invalid item no. for firm notification
#define	MDK_INFINITE_TIMEOUT	4000		// max time out for short command

#define	MDK_VERSION_LENGTH		32			// version string length

//	exception code
#define	MDK_EXCEPTION_NO_UNIT				0
#define	MDK_EXCEPTION_INVALID_PARAM			1
#define	MDK_EXCEPTION_CMD_ABORT				2
#define	MDK_EXCEPTION_CMD_FAILED			3
#define	MDK_EXCEPTION_UNIT_BUSY				4
#define	MDK_EXCEPTION_INVALID_OBJECT		5
#define	MDK_EXCEPTION_MEMORY				6

//-----------------------------------------------------------------------------
//	for old VC++
#if _MSC_VER >= 1400
//#define	STRCPY(d, len, s)	strcpy_s((d), (len), (s));
#define	STRCPY(d, len, s)	_tcscpy_s((d), (len), (s));
#define	STRCAT(d, len, s)	_tcscat_s((d), (len), (s));
#else
#define	STRCPY(d, len, s)	_tcscpy((d), (s));
#define	STRCAT(d, len, s)	_tcscat((d), (s));
#endif	// _MSC_VER

#endif	//	__MDK_DEF_H__
