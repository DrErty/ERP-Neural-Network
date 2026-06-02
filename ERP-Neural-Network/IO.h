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
}