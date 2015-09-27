// A C++11 toy garbage collector for trivial types using RAII

#include <cstddef>
#include <iostream>
#include <iomanip>
#include <map>
#include <type_traits>
#include <utility>

class GC // RAII Garbage Collector class for PODs
{
    // memory pool
    static std::map<void*, std::pair<std::size_t, bool>> _memory;
public:
    struct collect_t {}; // tag for placement new
    static void display_memory(); // displays the managed memory
    static void clear_memory(); // clears the managed memory
    static bool is_managed(void* p); // true iff p is managed
    static void remove(void *p); // remove p if managed, no-op otherwise
    // perfect candidate for an optional return type
    // returns p if mangaged, (0,false) otherwise
    static std::pair<std::size_t, bool> get(void* p);
    ~GC(); // clears the memory
// friends:
    friend void* operator new(std::size_t size, const GC::collect_t&,
                              bool is_array);
    friend void* operator new[](std::size_t size, const GC::collect_t&);
    friend void operator delete(void *p, const GC::collect_t&) noexcept;
    friend void operator delete[](void *p, const GC::collect_t&) noexcept;
};
std::map<void*, std::pair<std::size_t, bool>> GC::_memory; // ODR

// tagged overloads
void* operator new(std::size_t size, const GC::collect_t&,
                   bool is_array = false);
void* operator new[](std::size_t size, const GC::collect_t&);
void operator delete(void *p, const GC::collect_t&) noexcept;
void operator delete[](void *p, const GC::collect_t&) noexcept;

#define NEW new(GC::collect_t{}) // managed new

struct Foo // a trivial class
{
    int x;
    char buf[256];
};

int main() // Testing
{
    GC gc; // RAII, calls GC::clear_memory() when out-of-scope
    std::cout << "std::is_trivial<Foo>::value: " << std::boolalpha
              << std::is_trivial<Foo>::value << std::endl;

    // use the garbage collector
    char *c = NEW char;
    int *p = NEW int[1024]; // true size: sizeof(int)*100 + y
    Foo *pFoo = NEW Foo;

    // don't use the garbage collector
    int *tmp = new int;

    std::cout << "GC::is_managed(): " << GC::is_managed(c) << std::endl;
    std::cout << "GC::is_managed(): " << GC::is_managed(p) << std::endl;
    std::cout << "GC::is_managed(): " << GC::is_managed(pFoo) << std::endl;
    std::cout << "GC::is_managed(): " << GC::is_managed(tmp) << std::endl;

    GC::display_memory();

    GC::remove(c);
    GC::display_memory();
    std::cout << "GC::is_managed(): " << GC::is_managed(c) << std::endl;
    std::cout << "GC::is_managed(): " << GC::is_managed(p) << std::endl;
    std::cout << "GC::is_managed(): " << GC::is_managed(pFoo) << std::endl;
    std::cout << "GC::is_managed(): " << GC::is_managed(tmp) << std::endl;

    auto pair = GC::get(p);
    if (pair.first != 0 && pair.second != false) // element is managed
    {
        std::cout << "GC::get(): " << pair.first
                  << " -> " << pair.second << std::endl;
    }

    delete tmp;
}

// Implementation

void GC::display_memory()
{
    std::cout << "Allocated: " << std::endl;
    for (auto && elem : _memory)
    {
        std::cout  << std::setfill('.')
                   << "\tADDR: "  << std::setw(8)
                   << elem.first
                   << "\tSIZE: " << std::setw(8) << std::right
                   << elem.second.first
                   << "\tARRAY: " << std::setw(8) << elem.second.second
                   << std::endl;
    }
}

void GC::clear_memory()
{
    for (auto && elem : _memory)
    {
        if (elem.second.second) // non-array
            operator delete(elem.first, GC::collect_t{});
        else
            operator delete[](elem.first, GC::collect_t{});
    }
    _memory.clear();
}

bool GC::is_managed(void* p)
{
    return _memory.find(p) != _memory.end();
}

void GC::remove(void *p)
{
    _memory.erase(p);
}

std::pair<std::size_t, bool> GC::get(void* p)
{
    if (is_managed(p))
        return _memory[p];
    else
        return std::make_pair(0, false);
}

GC::~GC()
{
    std::cout << "GC::~GC() releasing memory" << std::endl;
    clear_memory();
    display_memory();
}

void* operator new(std::size_t size, const GC::collect_t&, bool is_array)
{
    void* addr = ::operator new(size);
    GC::_memory[addr] = std::make_pair(size, is_array);
    return addr;
}

void* operator new[](std::size_t size, const GC::collect_t&)
{
    void* addr = ::operator new[](size);
    GC::_memory[addr] = std::make_pair(size, true);
    return addr;
}

void operator delete(void *p, const GC::collect_t&) noexcept
{
    GC::_memory.erase(p); // should call ::operator delete, no recursion
    ::operator delete(p);
}

void operator delete[](void *p, const GC::collect_t&) noexcept
{
    GC::_memory.erase(p); // should call ::operator delete, no recursion
    ::operator delete[](p);
}
