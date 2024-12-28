#include "registry.h"

#if REG_LEVEL >= 2

BOOL IsDoubleNullTerminated(const std::vector<TCHAR> data) // IsMultiString?
{
	if (data.size() < 2)
		return FALSE;
	const size_t last = data.size() - 1;
	return ((data[last] == _T('\0')) && (data[last - 1] == _T('\0'))) ? TRUE : FALSE;
}

size_t MultiStringToStrings(const std::vector<TCHAR> data, std::vector<std::tstring> &segments, BOOL skipEmpty)
{
	if (!IsDoubleNullTerminated(data))
		return 0;
	segments.clear();
	// parse
	const TCHAR *currentPtr = data.data();
	const TCHAR *const endPtr = data.data() + data.size() - 1;
	while (currentPtr < endPtr)
	{
		const size_t len = _tcslen(currentPtr); // StringCchLength
		if (len > 0)
			segments.emplace_back(currentPtr, len);
		else if (!skipEmpty)
			segments.emplace_back(std::tstring{}); // insert empty string
		currentPtr += len + 1;					   // nema byt namiesto jednotky +sizeof(TCHAR) ???
	}
	return segments.size(); // strings count
}

size_t StringsToMultiString(const std::vector<std::tstring> segments, std::vector<TCHAR> &data)
{
	data.clear();
	if (segments.empty())
		data.resize(2, _T('\0'));
	else
	{
		// get necessary length
		size_t total = 0;
		for (const auto &s : segments)
			total += s.length() + 1; // string len plus \0 char
		total++;					 // need for double \0
		data.reserve(total);
		// copy
		for (const auto &s : segments)
		{
			if (!s.empty())
				data.insert(data.end(), s.begin(), s.end());
			data.emplace_back(_T('\0'));
		}
		data.emplace_back(_T('\0')); // double \0
	}
	return data.size() * sizeof(TCHAR); // in bytes
}

#endif // REG_LEVEL
