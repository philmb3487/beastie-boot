#pragma once

#include <string_view>
#include <vector>

namespace beastie {
class CEnvironmentWriter
{
public:
    CEnvironmentWriter();

    void clear();
    auto data() {
        return m_buffer.data();
    }
    auto size() {
        return m_buffer.size();
    }

    void addString(std::string_view);
    void operator+=(std::string_view);

private:
    std::vector<char> m_buffer;
    int m_count;


private:
    void push(std::string_view);
};
} // namespace beastie
