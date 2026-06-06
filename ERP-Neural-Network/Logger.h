#pragma once

#include "Utils/StringUtil.h"

namespace Logger
{
	void PrintInternal(const char* str);

	template<typename... Args>
	static void PrintLine(Args... args)
	{
		StaticBuffer<char> buffer(2048 * 4);
		StringBuilder stringBuilder(buffer);

		(stringBuilder.Concat(args), ...);

		stringBuilder.Concat('\n');
		stringBuilder.Compile();

		PrintInternal(buffer.GetData());
	}
};