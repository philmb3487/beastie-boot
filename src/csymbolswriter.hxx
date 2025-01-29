#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#include <concepts>

namespace beastie {
class CSymbolsWriter
{
public:
    CSymbolsWriter();

    void clear();
    auto data() {
        return m_buffer.data();
    }
    auto size() {
        return m_buffer.size();
    }

    void addSymTab(std::vector<char>);
    void addStrTab(std::vector<char>);

private:
    std::vector<char> m_buffer;


private:
    void push(std::integral auto);
    void push(std::vector<char>);
    void align(size_t);
    size_t offset() {
        return (m_buffer.end() - m_buffer.begin());
    }
};
} // namespace beastie
