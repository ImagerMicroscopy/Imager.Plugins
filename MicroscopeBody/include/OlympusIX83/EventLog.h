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

#pragma once

#include <clocale>

#ifdef	_GT_LOG
#define	DLL_LOG_DECLARE		__declspec(dllexport)
#else
#define	DLL_LOG_DECLARE		__declspec(dllimport)
#endif

//-------------------------------------------------------------------
//	constants
#define	GT_LOG_INI_FILE		_T("gtlog.ini")
#define	GT_LOG_DEF_COLUMNS	128
#define	GT_LOG_DEF_LINES	2048
#define	GT_LOG_DEF_LEVEL	5

#define	GT_LOG_MAX_COLUMNS	256
#define	GT_LOG_MAX_LINES	4096

#ifdef WIN32
#define CRLF_LENGTH			2
#else
#define	CRLF_LENGTH			1
#endif

// log level..
#define	EV_ERROR			1		// error
#define	EV_ENTRY			2		// function entry (enter or exit)
#define	EV_WARNING			3		// warning
#define	EV_MEMORY			3		// memory status

#define	EV_INFO1			4		// info lv1
#define	EV_INFO2			5		// info lv2
#define	EV_INFO3			6		// info lv3
#define	EV_INFO4			7		// info lv4 (most detail)

#define	EV_TINY_ENTRY		5
#define	EV_TINY1			6
#define	EV_TINY2			7

//-------------------------------------------------------------------
//	macro
#define	GTLOG(lv, ...)	theApp.Ev->Print((lv), __VA_ARGS__)
#define	PGTLOG(lv, ...)	ptheApp->Ev->Print((lv), __VA_ARGS__)
#define	GTERR(lv, ...)	theApp.EvErr->Print((lv), __VA_ARGS__)

//-------------------------------------------------------------------
//	event class
class DLL_LOG_DECLARE	CEventLog
{
	//////////////////////////////////////////////////////////
	// variables
	TCHAR		m_Path[_MAX_FNAME];	// path
	TCHAR		m_Mod[_MAX_FNAME];	// module
	TCHAR		m_fn[_MAX_FNAME];	// file name
	long		m_columns;			// log length in 1 line
	long		m_lines;			// log lines
	long		m_current;			// current line
	long		m_level;			// log print level
	long		m_fid;				// file ID (obsolete)
	int			m_Trace;			// trace output (for DEBUGGER)
	int			m_Erase;			// log erase when restart.
	TCHAR		m_LibPath[_MAX_FNAME];

public:
	//////////////////////////////////////////////////////////
	// implementation
	CEventLog(
		LPCTSTR	module,
		long	fid,
		long	columns = GT_LOG_DEF_COLUMNS,
		long	lines = GT_LOG_DEF_LINES,
		long	level = GT_LOG_DEF_LEVEL);

	~CEventLog(void);
	int	Clear();
	int	Print(
		const int	level,		// log level
		LPCTSTR		msg, ...);

#ifdef	_UNICODE
	int Print(
		const int	level,		// log level
		LPCSTR		msg, ...);
#endif

protected:
	_locale_t	m_Loc;
	int	AdjustString(
		OUT char		*dst,
		IN int			buf_size,
		IN const char	*src,
		int				length);
	bool	GetLibPath();
};

