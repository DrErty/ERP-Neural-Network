#pragma once

#include "ERP.h"

#include "ContinuousNetwork.h"

namespace IO
{
    struct GenomeSaveResult
    {
        bool Success = false;
        std::string Error;
        explicit operator bool() const { return Success; }
    };

    struct GenomeLoadResult
    {
        bool Success = false;
        NetworkGenome Genome = {};
        std::string Error;
        explicit operator bool() const { return Success; }
    };

    GenomeSaveResult SaveGenome(const NetworkGenome& genome, std::string_view fileName);
    GenomeLoadResult LoadGenome(std::string_view fileName);

    template <typename... Buffers>
    bool SaveCsv(const char* path,
        std::initializer_list<std::string_view> headers,
        Buffers&&... buffers)
    {
        constexpr std::size_t N = sizeof...(buffers);
        std::array<std::span<const Scalar>, N> cols{ std::span<const Scalar>(buffers)... };

        std::ofstream file(path);
        if (!file) return false;

        file.precision(std::numeric_limits<Scalar>::max_digits10);

        std::size_t h = 0;
        for (std::string_view name : headers)
            file << (h++ ? "," : "") << name;
        if (headers.size()) file << '\n';

        std::size_t rows = 0;
        if constexpr (N > 0)
        {
            rows = cols[0].size();
            for (const auto& c : cols)
                rows = std::min(rows, c.size());
        }

        for (std::size_t r = 0; r < rows; ++r)
        {
            for (std::size_t c = 0; c < N; ++c)
                file << (c ? "," : "") << cols[c][r];
            file << '\n';
        }

        return true;
    }

    template <typename... Buffers>
    bool SaveCsv(const char* path, Buffers&&... buffers)
    {
        return saveCsv(path, {}, std::forward<Buffers>(buffers)...);
    }
}