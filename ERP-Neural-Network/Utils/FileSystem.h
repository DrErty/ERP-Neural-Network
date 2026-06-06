#pragma once

#include "ERP.h"

#include <fstream>

#include "Memory/StaticBuffer.h"

namespace FileSystem
{
	template<typename T> requires std::is_trivially_constructible_v<T>
	StaticBuffer<T> ReadFile(const char* filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		ERP_VERIFY(file.is_open(), "Failed to load file.");

		const size_t size = static_cast<size_t>(file.tellg());

		ERP_ASSERT(size % sizeof(T) == 0, "Size of file must be a multiple of the size of the type.");

		StaticBuffer<T> buffer(size / sizeof(T));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.GetData()), buffer.GetCount() * sizeof(T));

		file.close();

		return buffer;
	}

	template<typename T> requires std::is_trivially_constructible_v<T>
	struct ReadResult
	{
		bool Success = false;
		StaticBuffer<T> Buffer{};
	};

	template<typename T> requires std::is_trivially_constructible_v<T>
	ReadResult<T> AttemptReadFile(const char* filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		ReadResult<T> result{};

		if (!file.is_open())
		{
			return result;
		}

		const size_t size = static_cast<size_t>(file.tellg());

		ERP_ASSERT(size % sizeof(T) == 0, "Size of file must be a multiple of the size of the type.");

		result.Buffer.Allocate(size / sizeof(T));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(result.Buffer.GetData()), result.Buffer.GetCount() * sizeof(T));

		file.close();

		result.Success = true;

		return result;
	}

	static std::string ReadFileIntoString(const char* filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		ERP_VERIFY(file.is_open(), "Failed to load file.");

		const size_t size = static_cast<size_t>(file.tellg());

		std::string buffer(size, '\0');

		file.seekg(0);
		file.read(buffer.data(), buffer.size());

		file.close();

		return buffer;
	}

	static void WriteFile(const char* filePath, const void* data, size_t size)
	{
		std::ofstream file(filePath, std::ios::out | std::ios::binary);
		
		file.seekp(0);
		file.write(static_cast<const char*>(data), size);
		file.flush();

		file.close();
	}
}