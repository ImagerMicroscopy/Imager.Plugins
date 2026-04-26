#ifndef UTILS_H
#define UTILS_H

#include <functional>
#include <string>
#include <tuple>

#include "windows.h"

class ScopedRunner {
public:
	ScopedRunner(std::function<void()> f) : _f(f) {}
	~ScopedRunner() { _f(); }
private:
	std::function<void()> _f;
};

// https://stackoverflow.com/questions/6284524/bstr-to-stdstring-stdwstring-and-vice-versa/6284978
std::string ConvertWCSToMBS(const wchar_t* pstr, int wslen) {
	int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

	std::string dblstr(len, '\0');
	len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
		pstr, wslen /* not necessary NULL-terminated */,
		&dblstr[0], len,
		NULL, NULL /* no default char */);

	return dblstr;
}

std::string ConvertWCSToMBS(const wchar_t* pstr) {
    return ConvertWCSToMBS(pstr, static_cast<int>(wcslen(pstr)));
}

std::string BStrToStdString(const bstr_t& str) {
	int wslen = str.length();
	return ConvertWCSToMBS((const wchar_t*)str, wslen);
}

#endif
