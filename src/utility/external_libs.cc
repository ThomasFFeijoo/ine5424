// Source code from external libs implementation

#include <utility/external_libs.h>

__BEGIN_UTIL

// src https://searchcode.com/codesearch/view/10260157/
bool Helper::isspace(unsigned char c) {
    if ( c == ' '
        || c == '\f'
        || c == '\n'
        || c == '\r'
        || c == '\t'
        || c == '\v' )
        return true;

    return false;
}

// src https://searchcode.com/codesearch/view/10200599/
bool Helper::isdigit(unsigned char c) {
    if ( c >= '0' && c <= '9' )
        return true;

    return false;
}

__END_UTIL
