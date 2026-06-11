#include "IO.h"

namespace IO
{
    static std::string ToStr(Scalar v)
    {
        std::array<char, 32> buf = {};
        const auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + sizeof(buf), v);
        return std::string(buf.data());
    }

    static std::vector<std::string> SplitLine(const std::string& line)
    {
        std::vector<std::string> out;
        std::string cur;
        for (char c : line)
        {
            if (c == ',') { out.push_back(std::move(cur)); cur.clear(); }
            else if (c != '\r')  cur.push_back(c);
        }
        out.push_back(std::move(cur));

        for (std::string& token : out)
        {
            const size_t b = token.find_first_not_of(" \t");
            const size_t e = token.find_last_not_of(" \t");
            token = (b == std::string::npos) ? std::string{} : token.substr(b, e - b + 1);
        }
        return out;
    }

    template <typename T>
    bool TryParse(std::string_view token, T& out)
    {
        const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), out);
        return ec == std::errc{} && ptr == token.data() + token.size();
    }

    GenomeSaveResult SaveGenome(const NetworkGenome& genome, std::string_view fileName)
    {
        auto fail = [&](const std::string& reason) -> GenomeSaveResult
            {
                return { false, "GenomeIO: '" + std::string(fileName) + "': " + reason };
            };

        std::ofstream file(std::string(fileName), std::ios::out | std::ios::trunc);
        if (!file)
            return fail("could not open file for writing");

        file.imbue(std::locale::classic());

        file << "layers";
        for (uint32_t i = 0; i < LAYER_COUNT; ++i)
            file << ',' << LAYER_SIZES[i];
        file << '\n';

        file << "weights";
        for (Scalar w : genome.Weights)
            file << ',' << ToStr(w);
        file << '\n';

        file << "biases";
        for (Scalar b : genome.Biases)
            file << ',' << ToStr(b);
        file << '\n';

        if (!file)
            return fail("write failed");

        return { true, {} };
    }

    GenomeLoadResult LoadGenome(std::string_view fileName)
    {
        auto fail = [&](const std::string& reason) -> GenomeLoadResult
            {
                return { false, {}, "GenomeIO: '" + std::string(fileName) + "': " + reason };
            };

        std::ifstream file(std::string(fileName), std::ios::in);
        if (!file)
            return fail("could not open file for reading");

        NetworkGenome genome;
        std::string line;

        if (!std::getline(file, line))
            return fail("file is empty");

        std::vector<std::string> tok = SplitLine(line);
        if (tok.empty() || tok[0] != "layers")
            return fail("missing 'layers' header line");
        if (tok.size() - 1 != LAYER_COUNT)
            return fail("topology mismatch: file has " + std::to_string(tok.size() - 1) +
                " layers, this build expects " + std::to_string(LAYER_COUNT));
        for (uint32_t i = 0; i < LAYER_COUNT; ++i)
        {
            uint32_t size = 0;
            if (!TryParse(tok[i + 1], size))
                return fail("could not parse layer size '" + tok[i + 1] + "'");
            if (size != LAYER_SIZES[i])
                return fail("topology mismatch at layer " + std::to_string(i) +
                    ": file has " + std::to_string(size) +
                    ", this build expects " + std::to_string(LAYER_SIZES[i]));
        }

        if (!std::getline(file, line))
            return fail("missing weights line");
        tok = SplitLine(line);
        if (tok.empty() || tok[0] != "weights")
            return fail("missing 'weights' label");
        if (tok.size() - 1 != WEIGHT_COUNT)
            return fail("expected " + std::to_string(WEIGHT_COUNT) + " weights, found " +
                std::to_string(tok.size() - 1));
        for (uint32_t i = 0; i < WEIGHT_COUNT; ++i)
            if (!TryParse(tok[i + 1], genome.Weights[i]))
                return fail("could not parse weight " + std::to_string(i));

        if (!std::getline(file, line))
            return fail("missing biases line");
        tok = SplitLine(line);
        if (tok.empty() || tok[0] != "biases")
            return fail("missing 'biases' label");
        if (tok.size() - 1 != BIAS_COUNT)
            return fail("expected " + std::to_string(BIAS_COUNT) + " biases, found " +
                std::to_string(tok.size() - 1));
        for (uint32_t i = 0; i < BIAS_COUNT; ++i)
            if (!TryParse(tok[i + 1], genome.Biases[i]))
                return fail("could not parse bias " + std::to_string(i));

        return { true, genome, {} };
    }
}