#ifndef HEADER_REGISTRY
#define HEADER_REGISTRY

#define REG_LEVEL 0 // 0 = only numbers / 1 = numbers + strings / 2 = numbers + multi strings

#if defined(_UNICODE) && !defined(UNICODE)
#define UNICODE // used by Windows headers
#endif
#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE // used by C-runtime
#endif

#include <windows.h>

#if REG_LEVEL >= 1
#include <tchar.h>
#if REG_LEVEL >= 2
#include <string>
#include <vector>

namespace std
{
    #ifdef _UNICODE
        typedef wstring tstring;
    #else
        typedef string tstring;
    #endif
};

BOOL IsDoubleNullTerminated(const std::vector<TCHAR> data); // IsMultiString?
size_t MultiStringToStrings(const std::vector<TCHAR> data, std::vector<std::tstring> &segments, BOOL skipEmpty = TRUE);
size_t StringsToMultiString(const std::vector<std::tstring> segments, std::vector<TCHAR> &data);
#endif
#endif

class RegKey
{
	HKEY hKey = NULL; // opened reg key handle
public:
	RegKey();																	// empty constructor (for non exist reg key)
	RegKey(HKEY hKeyRoot, LPCTSTR lpKey, REGSAM access_rights = KEY_ALL_ACCESS) // for determine if key exists, just open with KEY_ENUMERATE_SUB_KEYS and check IsOpen
	{
		this->Open(hKeyRoot, lpKey, access_rights);
	}

	~RegKey() // close
	{
		this->Close();
	}

	void Close()
	{
		RegCloseKey(this->hKey);
		this->hKey = NULL;
	}

	BOOL Open(HKEY hKeyRoot, LPCTSTR lpKey, REGSAM access_rights = KEY_ALL_ACCESS)
	{
		if (this->hKey)
			this->Close();
		if (RegOpenKeyEx(hKeyRoot, lpKey, 0, access_rights, &this->hKey) == ERROR_SUCCESS)
			return TRUE;
		else
			this->hKey = NULL;
		return FALSE;
	}

	BOOL OpenCreate(HKEY hKeyRoot, LPCTSTR lpKey, REGSAM access_rights = KEY_ALL_ACCESS)
	{
		if (this->hKey)
			this->Close();
		if (RegCreateKeyEx(hKeyRoot, lpKey, 0, NULL, REG_OPTION_NON_VOLATILE, access_rights, NULL, &this->hKey, NULL) == ERROR_SUCCESS)
			return TRUE;
		else
			this->hKey = NULL;
		return FALSE;
	}

	BOOL IsOpen()
	{
		return this->hKey ? TRUE : FALSE;
	}

	DWORD GetDword(LPCTSTR lpValueName, DWORD dwDefault = 0, LPCTSTR lpSubKey = NULL) // 32-bit number
	{
		DWORD data = dwDefault;
		DWORD dataSize = sizeof(data);
		RegGetValue(this->hKey, lpSubKey, lpValueName, RRF_RT_REG_DWORD, NULL, &data, &dataSize);
		return data;
	}

	ULONGLONG GetQword(LPCTSTR lpValueName, ULONGLONG qwDefault = 0, LPCTSTR lpSubKey = NULL) // 64-bit number
	{
		ULONGLONG data = qwDefault;
		DWORD dataSize = sizeof(data);
		RegGetValue(this->hKey, lpSubKey, lpValueName, RRF_RT_REG_QWORD, NULL, &data, &dataSize);
		return data;
	}

	BOOL SetDword(LPCTSTR lpValueName, const DWORD data = 0)
	{
		if (RegSetValueEx(this->hKey, lpValueName, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&data), sizeof(data)) == ERROR_SUCCESS)
			return TRUE; //(CONST BYTE*)&data
		return FALSE;
	}

	BOOL SetQword(LPCTSTR lpValueName, const ULONGLONG data = 0)
	{
		if (RegSetValueEx(this->hKey, lpValueName, 0, REG_QWORD, reinterpret_cast<const BYTE *>(&data), sizeof(data)) == ERROR_SUCCESS)
			return TRUE; //(CONST BYTE*)&data
		return FALSE;
	}

#if REG_LEVEL >= 1
	DWORD GetString(LPCTSTR lpValueName, LPTSTR lpString, DWORD nMaxCount, LPCTSTR lpSubKey = NULL) // null-terminated string
	{
		DWORD dataSize = 0;
		if (RegGetValue(this->hKey, lpSubKey, lpValueName, RRF_RT_REG_SZ, NULL, NULL, &dataSize) == ERROR_SUCCESS) // get size in bytes includes terminating \0 character
		{
			if (dataSize <= nMaxCount)
			{
				if (RegGetValue(this->hKey, lpSubKey, lpValueName, RRF_RT_REG_SZ, NULL, (PVOID)lpString, &dataSize) == ERROR_SUCCESS)
					return dataSize; // get data
			}
		}
		return 0;
	}

	BOOL SetString(LPCTSTR lpValueName, LPCTSTR data)
	{
		const DWORD dataSize = static_cast<DWORD>((_tcslen(data) + 1) * sizeof(TCHAR)); // static_cast<DWORD>((data.length() + 1) * sizeof(wchar_t));
		if (RegSetValueEx(this->hKey, lpValueName, 0, REG_SZ, reinterpret_cast<const BYTE *>(&data), dataSize) == ERROR_SUCCESS)
			return TRUE;
		return FALSE;
	}

#if REG_LEVEL >= 2
	DWORD GetStringMulti(LPCTSTR lpValueName, std::vector<std::tstring> &paths, LPCTSTR lpSubKey = NULL) // sequence of null-terminated strings terminated by an empty string (\0)
	{
		DWORD dataSize = 0;
		if (RegGetValue(this->hKey, lpSubKey, lpValueName, RRF_RT_REG_MULTI_SZ, NULL, NULL, &dataSize) == ERROR_SUCCESS) // get size in bytes includes every \0 character
		{
			std::vector<TCHAR> buffer(dataSize / sizeof(TCHAR)); // or std::vector<BYTE> buffer(dataSize);
			if (RegGetValue(this->hKey, lpSubKey, lpValueName, RRF_RT_REG_MULTI_SZ, NULL, &buffer[0], &dataSize) == ERROR_SUCCESS)
				return (DWORD)MultiStringToStrings(buffer, paths);
		}
		return 0;
	}

	BOOL SetStringMulti(LPCTSTR lpValueName, std::vector<std::tstring> &paths)
	{
		std::vector<TCHAR> data;
		DWORD dataSize = (DWORD)StringsToMultiString(paths, data);
		if (RegSetValueEx(this->hKey, lpValueName, 0, REG_MULTI_SZ, reinterpret_cast<const BYTE *>(data.data()), dataSize) == ERROR_SUCCESS)
			return TRUE;
		return FALSE;
	}
#endif // REG_LEVEL >= 2
#endif // REG_LEVEL >= 1

	DWORD GetValuesCount()
	{
		DWORD cValues = 0;
		if (RegQueryInfoKey(this->hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
			return cValues;
		return 0;
	}

	DWORD GetSubKeysCount()
	{
		DWORD cSubKeys = 0;
		if (RegQueryInfoKey(this->hKey, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
			return cSubKeys;
		return 0;
	}

	DWORD GetValueType(DWORD dwIndex)
	{
		DWORD dwLen = 0;
		DWORD dwType = 0;
		if (RegEnumValue(this->hKey, dwIndex, NULL, &dwLen, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
			return dwType;
		return 0;
	}

	DWORD GetValueName(DWORD dwIndex, LPTSTR lpName, DWORD nMaxCount)
	{
		DWORD dwLen = 0;
		DWORD dwType = 0;
		if (RegEnumValue(this->hKey, dwIndex, NULL, &dwLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwLen < nMaxCount)
			{
				if (RegEnumValue(this->hKey, dwIndex, lpName, &dwLen, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
				{
					lpName[dwLen] = '\0';
					return dwType; // or dwLen
				}
			}
		}
		return 0;
	}

	DWORD GetSubKeyName(DWORD dwIndex, LPTSTR lpName, DWORD nMaxCount)
	{
		DWORD dwLen = 0;
		if (RegEnumKeyEx(this->hKey, dwIndex, NULL, &dwLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwLen < nMaxCount)
			{
				if (RegEnumKeyEx(this->hKey, dwIndex, lpName, &dwLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
				{
					lpName[dwLen] = '\0';
					return dwLen;
				}
			}
		}
		return 0;
	}

	BOOL DeleteValue(LPCTSTR lpValueName)
	{
		if (RegDeleteValue(this->hKey, lpValueName) == ERROR_SUCCESS)
			return TRUE;
		return FALSE;
	}

#if _WIN32_WINNT >= 0x0600
	BOOL DeleteSubKey(LPCTSTR lpSubKey)
	{
		if (RegDeleteTree(this->hKey, lpSubKey) == ERROR_SUCCESS) // define _WIN32_WINNT as 0x0600 or later
			return TRUE;
		return FALSE;
	}
#endif
};

#endif // header guard
