#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#include <concepts>
#include <span>

namespace beastie {
class CMetaWriter
{
public:
    CMetaWriter();

    void clear();
    auto data() {
        return m_buffer.data();
    }
    auto size() {
        return m_buffer.size();
    }
    std::span<char> span() {
        return std::span<char>(m_buffer.begin(), m_buffer.end());
    }

    void addEnd();
    void addName(std::string_view);
    void addType(std::string_view);
    void addArgs(std::string_view);
    void addAddr(uintptr_t);
    void addSize(size_t);

    void addMetadata(int type, std::integral auto);
    void addMetadata(int type, std::span<char>);

private:
    std::vector<char> m_buffer;


private:
    void push(std::integral auto);
    void push(std::string_view);
    void str(auto, std::string_view);
    void var(auto, std::integral auto);
    void span(auto, std::span<char>);
    void align(size_t);
    size_t offset() {
        return (m_buffer.end() - m_buffer.begin());
    }
};
} // namespace beastie
