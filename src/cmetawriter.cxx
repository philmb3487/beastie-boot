#include "cmetawriter.hxx"
#include "constants.hxx"
using namespace beastie;

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <concepts>
#include <span>

constexpr static int ALIGN = sizeof(uintptr_t);

beastie::CMetaWriter::CMetaWriter()
    : m_buffer()
{
    m_buffer.reserve(4096);
    clear();
}

void beastie::CMetaWriter::clear()
{
    m_buffer.clear();
}

void beastie::CMetaWriter::push(std::integral auto v)
{
    m_buffer.resize(m_buffer.size() + sizeof(v));
    std::memcpy(m_buffer.end().base() - sizeof(v), &v, sizeof(v));
}

void beastie::CMetaWriter::str(auto type, std::string_view s)
{
    size_t size = (s.size() + 1);
    push(uint32_t(type));
    push(uint32_t(size));
    push(s);
    align(size);
}

void beastie::CMetaWriter::var(auto type, std::integral auto v)
{
    size_t size = (sizeof v);
    push(uint32_t(type));
    push(uint32_t(size));
    push(v);
    align(size);
}

void beastie::CMetaWriter::span(auto type, std::span<char> span)
{
    size_t size = span.size();
    push(uint32_t(type));
    push(uint32_t(size));
    for (char b : span)
        push(b);
    align(size);
}

void beastie::CMetaWriter::push(std::string_view str)
{
    size_t size = (str.size() + 1);
    m_buffer.resize(m_buffer.size() + size);
    std::memcpy(m_buffer.end().base() - size, str.data(), size);
}

void beastie::CMetaWriter::align(size_t a)
{
    while (offset() % ALIGN)
    {
        push(uint8_t(0));
    }
}

void beastie::CMetaWriter::addEnd()
{
    push(uint32_t(MODINFO_END));
    push(uint32_t(0));
}

void beastie::CMetaWriter::addName(std::string_view s)
{
    str(MODINFO_NAME, s);
}

void beastie::CMetaWriter::addType(std::string_view s)
{
    str(MODINFO_TYPE, s);
}

void beastie::CMetaWriter::addArgs(std::string_view s)
{
    str(MODINFO_ARGS, s);
}

void beastie::CMetaWriter::addAddr(uintptr_t a)
{
    var(MODINFO_ADDR, a);
}

void beastie::CMetaWriter::addSize(size_t s)
{
    var(MODINFO_SIZE, s);
}

void beastie::CMetaWriter::addMetadata(int type, std::integral auto v)
{
    var(type, v);
}

void beastie::CMetaWriter::addMetadata(int type, std::span<char> vs)
{
    span(type, vs);
}

template
void beastie::CMetaWriter::addMetadata<uint32_t>(int type, uint32_t v);

template
void beastie::CMetaWriter::addMetadata<uint64_t>(int type, uint64_t v);
