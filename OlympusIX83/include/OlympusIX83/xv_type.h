//-----------------------------------------------------------------------------
//
//  TITLE : XV definition
//	$Workfile: xv_type.h $ 
//	$Revision: 1 $ 
//	$JustDate: 09/04/17 $ 
//
//	Copyright 2008 - 2013 OLYMPUS CORPORATION All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef	__XV_TYPE__
#define	__XV_TYPE__

#include	<vector>
#include	<map>
#include	<string>

#pragma warning (disable: 4786) 

//-----------------------------------------------------------------------------
//	unit connection
typedef	enum  tag_eDeviceFlags { 
  e_DFNoFlag = 0,					// No flags defined.  
  e_DFPlugAndPlay = 1,				// This unit will be automatically detected by the driver.  
  e_DFAddManual = 2,				// No automatic detection of identical units possible. This flag allows multiple registration.  
  e_DFVirtualDevice = 8, 			// This flag indicates whether the unit is a virtual device.  
  e_DFSimulator = 16,				// This device simulates a real device.  
  e_DFManualDevice = 32 			// Non-motorized device like a manual nosepiece.  
} eDeviceFlags;

//-----------------------------------------------------------------------------
//	interface type
typedef	enum tag_eDeviceInterface { 
  e_DINoInterface = 1,				// No interface required.  
  e_DISerial = 2,					// COM port. 
  e_DIParallel = 3,					// LPT port.
  e_DIUSB = 4,						// USB
  e_DIFireWire = 5,					// IEEE 1394. 
  e_DIIEEE488 = 6,					// IEEE 488.
  e_DICAN = 7,						// CAN bus.
  e_DISCSI = 8,						// SCSI.
  e_DIIDE = 9,						// IDE.
  e_DISoftware = 10,				// TWAIN etc., driver but no hardware.  
  e_DIMisc = 11,					// PCI interface cards etc.
  e_DIDummyType = 12				// Dummy type for test.  
} eDeviceInterface;

//-----------------------------------------------------------------------------
//	result
struct	DLL_EXPORT	LDIResult {
	enum eLDIResultCode { 
		SUCCESS = 0,				// no error, operation succeeded  
		INTERNAL_CODE,				// internal error code (set in _internal_code)  
		ERROR_UNSPECIFIED,			// unspecified error, should be avoided to report  
		FUNC_NOT_SUPPORTED, 		// function not supported (e.g. driver does not support async operations)  
		PROP_NOT_SUPPORTED,			// application requested unsopported property  
		VALUE_OUT_OF_RANGE,			// a value is out of range  
		HW_NOT_AVAILABLE,			// operation cannot be started because the hardware is not connected  
		HW_EXTERNALY_LOCKED,		// operation failed, because another application or handswitch has already exclusive access  
		TIMEOUT,					// an operation did not finish  
		PENDING,					// Asynchronous command undergoing
		ABORT,						// command abort
		LPARAM_CODE,				// lParam Error (for Stage)
		NO_MEMORY,					// Memory Error (R18)
		EXPOSURE_ERROR				// AE error (R20)
	};

	LDIResult (eLDIResultCode ldi_result_code=LDIResult::SUCCESS, __int64 internal_code=0)
	{
		LDIResultCode	= ldi_result_code;
		InternalCode	= internal_code;
	};

	eLDIResultCode	LDIResultCode;
	__int64			InternalCode;
};

//-----------------------------------------------------------------------------
// callback information
typedef struct _tag_MDK_CALLBACK_INFO {
	ULONG				Id;							// callback ID
	GT_CALLBACK_TYPE	CbType;						// callback type
	GT_CALLBACK_ENTRY	pEntry;						// callback entry
	PVOID				pContext;					// callback context
	PVOID				pExt1;						// extend for user 1
	PVOID				pExt2;						// extend for user 2
	PVOID				pExt3;						// extend for user 3
} MDK_CALLBACK_INFO, *PMDK_CALLBACK_INFO;

#endif	// __XV_TYPE__

