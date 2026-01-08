//-----------------------------------------------------------------------------
//	MDK_IF.H
//		communication library header
//
//	Copyright 2008 - 2013 OLYMPUS CORPORATION All Rights Reserved.
//

#pragma	once

#ifdef MDK_MAL_EXPORTS
#define MDK_MAL_API __declspec(dllexport)
#else
#define MDK_MAL_API __declspec(dllimport)
#endif

typedef	void*	MSL_PD_CONTEXT;		// contrext for port driver

//=============================================================================
//	command interface

#define	SZ_SMALL					8			// 
#define	MAX_TAG_SIZE				32			// command tag size
#define	MAX_COMMAND_SIZE			256			// command buffer size
#define	MAX_RESPONSE_SIZE			256			// response buffer size
#define	MAX_CMD						32			// command queue size

//	command status
typedef	enum tag_eMDK_CmdStatus {
	e_Cmd_Init = 0,						// initial state
	e_Cmd_Queue,						// queued
	e_Cmd_Wait,							// waiting for response
	e_Cmd_Complete,						// completion
	e_Cmd_Timeout,						// timeout
	e_Cmd_Abort							// aborted
} eMDK_CmdStatus;

#define	GT_MDK_CMD_SIGNATURE		'GCMD'

//-----------------------------------------------------------------------------
//	compeltion additional status
typedef	enum	tag_eMDK_CompleteStatus {
	e_Complete_Normal = 0,				// normal
	e_Complete_RemoveAll,				// remove requested.
	e_Complete_Timeout,					// timeot
	e_Complete_Abort,					// aborted
	e_Complete_Error,					// error
} eMDK_CompleteStatus;

//-----------------------------------------------------------------------------
//	command block
typedef	struct	tag_MDK_MSL_CMD	{
	DWORD			m_Signature;		// command block signature

	//-------------------------------------------
	// basic fields
	eDeviceType		m_Type;				// unit type
	WORD			m_Sequence;			// command sequence
	char			m_From[SZ_SMALL];	// from
	char			m_To[SZ_SMALL];		// to
	eMDK_CmdStatus	m_Status;			// command statys
	DWORD			m_Result;			// result
	BOOL			m_Sync;				// sync or async?
	BOOL			m_Command;			// command or query? (refer command tag)
	BOOL			m_SendOnly;			// TRUE means NOT wait response.

	//-------------------------------------------
	// management field
	time_t			m_StartTime;		// start time
	time_t			m_FinishTime;		// end time
	DWORD			m_Timeout;			// time out (ms)

	void			*m_Callback;		// callback entry
	void			*m_Event;			// event info
	void			*m_Context;			// context
	UINT			m_TimerID;			// timer ID
	void			*m_PortContext;		// port info.

	//-------------------------------------------
	//	extend field (for indivisual command)
	ULONG			m_Ext1;				// extend info 1
	ULONG			m_Ext2;				// extend info 2
	ULONG			m_Ext3;				// extend info 3
	LONGLONG		m_lExt1;			// LONGLONG extend info 1
	LONGLONG		m_lExt2;			// LONGLONG extend info 2

	//-------------------------------------------
	// tag fields
	DWORD			m_TagSize;			// tag size
	BYTE			m_CmdTag[MAX_TAG_SIZE];

	//-------------------------------------------
	// command string
	DWORD			m_CmdSize;			// size of command string
	BYTE			m_Cmd[MAX_COMMAND_SIZE];

	//-------------------------------------------
	// response string
	DWORD			m_RspSize;			// size of response string
	BYTE			m_Rsp[MAX_RESPONSE_SIZE];

} MDK_MSL_CMD, *PMDK_MSL_CMD;

//=============================================================================
//	port interface

#define	MAX_PORT_NAME_LENGTH	128		//	port name length

#define	GT_MDK_PORT_SIGNATURE		'GPRT'

//=============================================================================
//	port driver interface

//-----------------------------------------------------------------------------
//	port dll name.
typedef	struct	tag_MSL_PD_NAME	{
	wchar_t			PortName[MAX_PORT_NAME_LENGTH];
} MSL_PD_NAME, *PMSL_PD_NAME;

//-----------------------------------------------------------------------------
//	port dll exports
typedef	DLL_IMPORT	int	(*fn_EnumPorts)(
	eDeviceInterface *IfType,			// interface type
	int				*Counts,			// interface num
	MSL_PD_NAME		*PortNames,			// port name buffer
	DWORD			*bufferCount		// buffer size
);

typedef	DLL_IMPORT	MSL_PD_CONTEXT	(*fn_OpenPort)(wchar_t *PortName);
typedef	DLL_IMPORT	int	(*fn_ClosePort)(MSL_PD_CONTEXT p);
typedef	DLL_IMPORT	int	(*fn_WritePort)(MSL_PD_CONTEXT p, MDK_MSL_CMD *cmd);
typedef	DLL_IMPORT	int	(*fn_ReadPort)(MSL_PD_CONTEXT p, char *buffer, DWORD buf_size, DWORD *rcv_size);
typedef	DLL_IMPORT	BOOL	(*fn_ConfigPort)(wchar_t *PortName, HWND hWnd);
typedef	DLL_IMPORT	int	(*fn_GetConnection)(MSL_PD_CONTEXT p);

//-----------------------------------------------------------------------------
//	port DLL information
typedef	struct	tag_MSL_PD_EXPORT	{
	wchar_t				m_DllName[MAX_STRING];		// DLL name
	HMODULE				m_hMod;						// module handle
	// function entries
	fn_EnumPorts		pfn_EnumPorts;
	fn_OpenPort			pfn_OpenPort;
	fn_ClosePort		pfn_ClosePort;
	fn_WritePort		pfn_WritePort;
	fn_ReadPort			pfn_ReadPort;
	fn_ConfigPort		pfn_ConfigPort;
	fn_GetConnection	pfn_GetConnection;
} MSL_PD_EXPORT, *PMSL_PD_EXPORT;

//=============================================================================
//	port manager exports

typedef	DLL_IMPORT	int	(*fn_MSL_PM_Initialize)();

typedef	DLL_IMPORT	int	(*fn_MSL_PM_EnumInterface)();

typedef	DLL_IMPORT	int	(*fn_MSL_PM_GetInterfaceInfo)(
	IN	int				iInterfaceIndex,	// interface index
	OUT	void			**pInterface		// interface class address location
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_OpenInterface)(
	IN	void			*pInterface			// interface class address 
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_CloseInterface)(
	IN	void			*pInterface			// interface class address 
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_ConfigInterface)(
	IN	void			*pInterface,		// interface class address 
	IN  HWND			hWnd				// window handle for setting interface
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_SendCommand)(
	IN	void			*pInterface,		// interface class address 
	IN  MDK_MSL_CMD		*cmd				// command structure.
);

typedef	DLL_IMPORT	bool (*fn_MSL_PM_RegisterCallback)(
	IN void				*pInterface,		// interface class address
	IN GT_MDK_CALLBACK	Cb_Complete,		// callback for command completion
	IN GT_MDK_CALLBACK	Cb_Notify,			// callback for notification
	IN GT_MDK_CALLBACK	Cb_Error,			// callback for error notification
	IN VOID				*pContext			// callback context.
);

