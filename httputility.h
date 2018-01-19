#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include <sstream>
#include <string>
#include <cctype>

char from_hex(char ch)
{
    return std::isdigit(ch) ? ch - '0' : std::tolower(ch) - 'a' + 10;
}

std::string url_decode(std::string text)
{
    char h;
    std::ostringstream escaped;
    escaped.fill('0');

    for (auto i = text.begin(), n = text.end(); i != n; ++i)
    {
        std::string::value_type c = (*i);

        if (c == '%')
        {
            if (i[1] && i[2])
            {
                h = from_hex(i[1]) << 4 | from_hex(i[2]);
                escaped << h;
                i += 2;
            }
        }
        else if (c == '+')
        {
            escaped << ' ';
        }
        else
        {
            escaped << c;
        }
    }

    return escaped.str();
}

#endif // HTTPUTILITY_H
