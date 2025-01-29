#include "csymbolswriter.hxx"
using namespace beastie;

#include <cassert>
#include <cstdint>
#include <cstring>
#include <concepts>

constexpr static int ALIGN = sizeof(uintptr_t);

beastie::CSymbolsWriter::CSymbolsWriter()
    : m_buffer()
{
    m_buffer.reserve(4096);
    clear();
}

void beastie::CSymbolsWriter::clear()
{
    m_buffer.clear();
}

void beastie::CSymbolsWriter::push(std::integral auto v)
{
    m_buffer.resize(m_buffer.size() + sizeof(v));
    std::memcpy(m_buffer.end().base() - sizeof(v), &v, sizeof(v));
}

void beastie::CSymbolsWriter::push(std::vector<char> vs)
{
    m_buffer.insert(m_buffer.end(), vs.begin(), vs.end());
}

void beastie::CSymbolsWriter::addSymTab(std::vector<char> vs)
{
    push(long(vs.size()));
    push(vs);
    align(sizeof(long));
}

void beastie::CSymbolsWriter::addStrTab(std::vector<char> vs)
{
    push(long(vs.size()));
    push(vs);
    align(sizeof(long));
}

void beastie::CSymbolsWriter::align(size_t a)
{
    while (offset() % ALIGN)
    {
        push(uint8_t(0));
    }
}
