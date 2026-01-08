//**************************************************************************
// Overview
//--------------------------------------------------------------------------
// 
// The prototype declaration, structure definition, fixed number definition 
// and others are contained in this file.
//
//	Copyright 2008 - 2013 OLYMPUS CORPORATION All Rights Reserved.
//
//**************************************************************************
//**************************************************************************
// NOTE
//--------------------------------------------------------------------------
//
// ÅETAB:4
//
//**************************************************************************/
//**************************************************************************
// Change History
//--------------------------------------------------------------------------
//
// Rev. 00.01.00 	8 JUNE, 2007 	
//
//**************************************************************************/
#ifndef _GT_TYPE_H_
#define _GT_TYPE_H_

//=============================================================================
//	simple type definition
typedef	unsigned long	GT_STATUS;			//
typedef	void*			PGT_OBJECT;

#ifdef	__WINDOWS__
typedef __int64		LONGLONG;
#else
typedef long long	LONGLONG;
#endif	// __WINDOWS__

//=============================================================================
//	strcuture definition


//=============================================================================
//	callback definition

typedef	unsigned long	GT_CALLBACK_ID;		//
typedef	void			GT_CALLBACK_CONTEXT, *PGT_CALLBACK_CONTEXT;

typedef	enum	_GT_CALLBACK_TYPE {
	GT_CB_COMPLETION = 0,					// to notify completion
	GT_CB_STATUS_CHANGE,					// to notify changed status
	GT_CB_ERROR,							// to notify error
	GT_CB_ALL,								// to notify all event
	GT_CB_OTHER								// reserved.
} GT_CALLBACK_TYPE;

typedef	int	(__stdcall *GT_CALLBACK_ENTRY)(
		ULONG		MsgId,			// Callback ID.
		ULONG		wParam,			// Callback parameter, it depends on callback event.
		ULONG		lParam,			// Callback parameter, it depends on callback event.
		PVOID		pv,				// Callback parameter, it depends on callback event.
		PVOID		pContext,		// This contains information on this call back function.
		PVOID		pCaller			// This is the pointer specified by a user in the requirement function.
);

typedef	GT_CALLBACK_ENTRY	GT_MDK_CALLBACK;

#endif	//	_GT_TYPE_H_

