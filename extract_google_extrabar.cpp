#include <iostream>

#include "ege/extract_google_extrabar.hpp"

#include "mio.hpp"

int main()
{
    mio::mmap_source jp("data/jordan peterson books.html");
    mio::mmap_source vg("data/van gogh paintings.html");

    ege::extrabar::Parse(std::string_view((char*)jp.data(),jp.size()), [&](auto title, auto href, auto src)    
    {
        std::cout << title << std::endl << href << std::endl << src << std::endl << std::endl;
    });

    ege::extrabar::Parse(std::string_view((char*)vg.data(), vg.size()), [&](auto title, auto href, auto src)
    {
        std::cout << title << std::endl << href << std::endl << src << std::endl << std::endl;
    });

    return 0;
}

