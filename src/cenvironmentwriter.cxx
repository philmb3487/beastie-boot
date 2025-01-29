#include "cenvironmentwriter.hxx"

#include <cstring>

beastie::CEnvironmentWriter::CEnvironmentWriter()
    : m_buffer()
{
    m_buffer.reserve(4096);
    clear();
}

void beastie::CEnvironmentWriter::clear()
{
    m_buffer.clear();
    m_count = 0;

    // keep the buffer double-terminated
    m_buffer.push_back(0);
    m_buffer.push_back(0);
}

void beastie::CEnvironmentWriter::push(std::string_view str)
{
    size_t size = (str.size() + 1);
    m_buffer.resize(m_buffer.size() + size);
    std::memcpy(m_buffer.end().base() - size - 2, str.data(), size);
}

void beastie::CEnvironmentWriter::addString(std::string_view str)
{
    push(str);
    this->m_count ++;
}

void beastie::CEnvironmentWriter::operator+=(std::string_view str)
{
    addString(str);
}
