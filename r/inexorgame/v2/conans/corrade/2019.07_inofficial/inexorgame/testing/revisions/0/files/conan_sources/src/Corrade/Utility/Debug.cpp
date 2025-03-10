/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Debug.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>

/* For isatty() on Unix-like systems */
#ifdef CORRADE_TARGET_UNIX
#include <unistd.h>

/* Node.js alternative to isatty() on Emscripten */
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <emscripten.h>

#elif defined(CORRADE_TARGET_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN

/* For isatty() on Windows */
#ifdef CORRADE_UTILITY_USE_ANSI_COLORS
#include <io.h>

/* WINAPI-based colored output on Windows */
#else
#include <windows.h>
#include <wincon.h>
#endif
#endif

#include "Corrade/Containers/EnumSet.hpp"
#include "Corrade/Utility/DebugStl.h"

namespace Corrade { namespace Utility {

namespace {

template<class T> inline void toStream(std::ostream& s, const T& value) {
    s << value;
}

template<> inline void toStream<Implementation::DebugOstreamFallback>(std::ostream& s, const Implementation::DebugOstreamFallback& value) {
    value.apply(s);
}

#if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
HANDLE streamOutputHandle(const std::ostream* s) {
    return s == &std::cout ? GetStdHandle(STD_OUTPUT_HANDLE) :
           s == &std::cerr ? GetStdHandle(STD_ERROR_HANDLE) :
           INVALID_HANDLE_VALUE;
}
#endif

#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
std::ostream* globalOutput = &std::cout;
#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
std::ostream* globalWarningOutput = &std::cerr;
#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
std::ostream* globalErrorOutput = &std::cerr;

#if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
Debug::Color globalColor = Debug::Color::Default;
#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
bool globalColorBold = false;
#endif

}

template<Debug::Color c, bool bold> Debug::Modifier Debug::colorInternal() {
    return [](Debug& debug) {
        if(!debug._output || (debug._flags & InternalFlag::DisableColors)) return;

        debug._flags |= InternalFlag::ColorWritten|InternalFlag::ValueWritten;
        #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        HANDLE h = streamOutputHandle(debug._output);
        if(h != INVALID_HANDLE_VALUE) SetConsoleTextAttribute(h,
            (debug._previousColorAttributes & ~(FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY)) |
            char(c) |
            (bold ? FOREGROUND_INTENSITY : 0));
        #else
        globalColor = c;
        globalColorBold = bold;
        constexpr const char code[] = { '\033', '[', bold ? '1' : '0', ';', '3', '0' + char(c), 'm', '\0' };
        *debug._output << code;
        #endif
    };
}

inline void Debug::resetColorInternal() {
    if(!_output || !(_flags & InternalFlag::ColorWritten)) return;

    _flags &= ~InternalFlag::ColorWritten;
    _flags |= InternalFlag::ValueWritten;
    #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    HANDLE h = streamOutputHandle(_output);
    if(h != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(h, _previousColorAttributes);
    #else
    if(_previousColor != Color::Default || _previousColorBold) {
        const char code[] = { '\033', '[', _previousColorBold ? '1' : '0', ';', '3', char('0' + char(_previousColor)), 'm', '\0' };
        *_output << code;
    } else *_output << "\033[0m";

    globalColor = _previousColor;
    globalColorBold = _previousColorBold;
    #endif
}

auto Debug::color(Color color) -> Modifier {
    /* Crazy but working solution to work around the need for capturing lambda
       which disallows converting it to function pointer */
    switch(color) {
        #define _c(color) case Color::color: return colorInternal<Color::color, false>();
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        _c(Default)
        #endif
        #undef _c
    }

    return [](Debug&) {}; /* LCOV_EXCL_LINE */
}

auto Debug::boldColor(Color color) -> Modifier {
    /* Crazy but working solution to work around the need for capturing lambda
       which disallows converting it to function pointer */
    switch(color) {
        #define _c(color) case Color::color: return colorInternal<Color::color, true>();
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        _c(Default)
        #endif
        #undef _c
    }

    return [](Debug&) {}; /* LCOV_EXCL_LINE */
}

void Debug::resetColor(Debug& debug) {
    debug.resetColorInternal();
}

namespace { enum: unsigned char { PublicFlagMask = 0x1f }; }

auto Debug::flags() const -> Flags {
    return Flag(static_cast<unsigned char>(_flags) & PublicFlagMask);
}

void Debug::setFlags(Flags flags) {
    _flags = InternalFlag(static_cast<unsigned char>(flags)) |
        InternalFlag(static_cast<unsigned char>(_flags) & ~PublicFlagMask);
}

auto Debug::immediateFlags() const -> Flags {
    return Flag(static_cast<unsigned char>(_immediateFlags) & PublicFlagMask) |
        Flag(static_cast<unsigned char>(_flags) & PublicFlagMask);
}

void Debug::setImmediateFlags(Flags flags) {
    /* unlike _flags, _immediateFlags doesn't contain any internal flags so
       no need to preserve these */
    _immediateFlags = InternalFlag(static_cast<unsigned char>(flags));
}

std::ostream* Debug::output() { return globalOutput; }
std::ostream* Warning::output() { return globalWarningOutput; }
std::ostream* Error::output() { return globalErrorOutput; }

bool Debug::isTty(std::ostream* const output) {
    /* On Windows with WINAPI colors check the stream output handle */
    #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    return streamOutputHandle(output) != INVALID_HANDLE_VALUE;

    /* We can autodetect via isatty() on Unix-like systems and Windows with
       ANSI colors enabled */
    #elif defined(CORRADE_UTILITY_USE_ANSI_COLORS) || defined(CORRADE_TARGET_UNIX)
    return
        /* Windows RT projects have C4996 treated as error by default. WHY */
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4996)
        #endif
        ((output == &std::cout && isatty(1)) ||
         (output == &std::cerr && isatty(2)))
        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif
        #ifdef CORRADE_TARGET_APPLE
        /* Xcode's console reports that it is a TTY, but it doesn't support
           colors. We have to check for the following undocumented environment
           variable instead. If set, then don't use colors. */
        && !std::getenv("XPC_SERVICE_NAME")
        #endif
        ;

    /* Emscripten isatty() is kinda broken ATM (1.37.1), until fixed we have to
       call into Node.js: https://github.com/kripken/emscripten/issues/4920.
       Originally the code was simply using EM_ASM_INT_V() twice inside the
       if(output == ) branches, but that was causing crashes in llc (exit code
       -11) since after 1.37.5, so I had to rewrite the code in a different
       way to make the linker survive it. It involves passing some parameters
       to the inline assembly which are using the $ identifier and that is
       triggering a Clang warning and everything is just fucking ugly. */
    #elif defined(CORRADE_TARGET_EMSCRIPTEN)
    int out = 0;
    if(output == &std::cout)
        out = 1;
    else if(output == &std::cerr)
        out = 2;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
    return EM_ASM_INT({
        if(typeof process !== 'undefined') {
            if($0 == 1)
                return process.stdout.isTTY;
            else if($0 == 2)
                return process.stderr.isTTY;
        }
        return false;
    }, out);
    #pragma GCC diagnostic pop

    /* Otherwise can't be autodetected, thus disable colkors by default */
    #else
    return false;
    #endif
}

bool Debug::isTty() { return isTty(globalOutput); }
bool Warning::isTty() { return Debug::isTty(globalWarningOutput); }
bool Error::isTty() { return Debug::isTty(globalErrorOutput); }

Debug::Debug(std::ostream* const output, const Flags flags): _flags{InternalFlag(static_cast<unsigned char>(flags))}, _immediateFlags{InternalFlag::NoSpace} {
    /* Save previous global output and replace it with current one */
    _previousGlobalOutput = globalOutput;
    globalOutput = _output = output;

    /* Save previous global color */
    #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    HANDLE h = streamOutputHandle(_output);
    if(h != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(h, &csbi);
        _previousColorAttributes = csbi.wAttributes;
    }
    #else
    _previousColor = globalColor;
    _previousColorBold = globalColorBold;
    #endif
}

Warning::Warning(std::ostream* const output, const Flags flags): Debug{flags} {
    /* Save previous global output and replace it with current one */
    _previousGlobalWarningOutput = globalWarningOutput;
    globalWarningOutput = _output = output;
}

Error::Error(std::ostream* const output, const Flags flags): Debug{flags} {
    /* Save previous global output and replace it with current one */
    _previousGlobalErrorOutput = globalErrorOutput;
    globalErrorOutput = _output = output;
}

Debug::Debug(const Flags flags): Debug{globalOutput, flags} {}
Warning::Warning(const Flags flags): Warning{globalWarningOutput, flags} {}
Error::Error(const Flags flags): Error{globalErrorOutput, flags} {}

void Debug::cleanupOnDestruction() {
    /* Reset output color */
    resetColorInternal();

    /* Newline at the end */
    if(_output && (_flags & InternalFlag::ValueWritten) && !(_flags & InternalFlag::NoNewlineAtTheEnd))
        *_output << std::endl;

    /* Reset previous global output */
    globalOutput = _previousGlobalOutput;
}

Debug::~Debug() {
    cleanupOnDestruction();
}

Warning::~Warning() {
    globalWarningOutput = _previousGlobalWarningOutput;
}

void Error::cleanupOnDestruction() {
    globalErrorOutput = _previousGlobalErrorOutput;
}

Error::~Error() {
    cleanupOnDestruction();
}

Fatal::~Fatal() {
    /* Manually call cleanup of Error and Debug superclasses because their
       destructor will never be called */
    Error::cleanupOnDestruction();
    Debug::cleanupOnDestruction();

    std::exit(_exitCode);
}

template<class T> Debug& Debug::print(const T& value) {
    if(!_output) return *this;

    /* Separate values with spaces if enabled; reset all internal flags after */
    if(!((_immediateFlags|_flags) & InternalFlag::NoSpace))
        *_output << ' ';
    _immediateFlags = {};

    toStream(*_output, value);

    _flags |= InternalFlag::ValueWritten;
    return *this;
}

Debug& Debug::operator<<(const void* const value) {
    std::ostringstream o;
    o << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(value);
    return print(o.str());
}

Debug& Debug::operator<<(const char* value) { return print(value); }
Debug& Debug::operator<<(bool value) { return print(value ? "true" : "false"); }
Debug& Debug::operator<<(int value) { return print(value); }

Debug& Debug::operator<<(char value) { return print(int(value)); }

Debug& Debug::operator<<(unsigned char value) {
    /* Convert to int to avoid infinite recursion to operator<<(unsigned char) */
    const int v = value;

    /* Print the value as a shade of grey */
    if(immediateFlags() & Flag::Color) {
        const char* shade;
        if(value < 51)       shade = "  ";
        else if(value < 102) shade = "░░";
        else if(value < 153) shade = "▒▒";
        else if(value < 204) shade = "▓▓";
        else                 shade = "██";

        /* If ANSI colors are disabled, use just the shade */
        if(immediateFlags() & Flag::DisableColors)
            return print(shade);
        else {
            print("\033[38;2;");

            /* Disable space between values for everything after the initial
               value */
            const Flags previousFlags = flags();
            setFlags(previousFlags|Flag::NoSpace);

            /* Set both background and foreground, reset back after */
            *this << v << ";" << v << ";" << v << "m\033[48;2;"
                << v << ";" << v << ";" << v << "m" << shade << "\033[0m";

            /* Reset original flags */
            setFlags(previousFlags);
            return *this;
        }

    /* Otherwise print its numeric value */
    } else return print(v);
}

Debug& Debug::operator<<(long value) { return print(value); }
Debug& Debug::operator<<(long long value) { return print(value); }
Debug& Debug::operator<<(unsigned value) { return print(value); }
Debug& Debug::operator<<(unsigned long value) { return print(value); }
Debug& Debug::operator<<(unsigned long long value) { return print(value); }
Debug& Debug::operator<<(float value) {
    if(!_output) return *this;
    /* The default. Source: http://en.cppreference.com/w/cpp/io/ios_base/precision,
       Wikipedia says 6-digit number can be converted back and forth without
       loss: https://en.wikipedia.org/wiki/Single-precision_floating-point_format
       Kept in sync with format(). */
    *_output << std::setprecision(6);
    return print(value);
}
Debug& Debug::operator<<(double value) {
    if(!_output) return *this;
    /* Wikipedia says 15-digit number can be converted back and forth without
       loss: https://en.wikipedia.org/wiki/Double-precision_floating-point_format
       Kept in sync with format(). */
    *_output << std::setprecision(15);
    return print(value);
}
#ifndef CORRADE_TARGET_EMSCRIPTEN
Debug& Debug::operator<<(long double value) {
    if(!_output) return *this;
    /* Wikipedia says 18-digit number can be converted both ways without
       loss: https://en.wikipedia.org/wiki/Extended_precision#Working_range
       Kept in sync with format(). */
    *_output << std::setprecision(18);
    return print(value);
}
#endif

Debug& Debug::operator<<(char32_t value) {
    std::ostringstream o;
    o << "U+" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << value;
    return print(o.str());
}

Debug& Debug::operator<<(const char32_t* value) {
    return *this << std::u32string(value);
}

Debug& Debug::operator<<(std::nullptr_t) {
    return print("nullptr");
}

#ifndef DOXYGEN_GENERATING_OUTPUT
/* Doxygen can't match this to the declaration, eh. */
Debug& operator<<(Debug& debug, Debug::Color value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Debug::Color::value: return debug << "Debug::Color::" #value;
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        _c(Default) /* Alias to White on Windows */
        #endif
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Debug::Color(" << Debug::nospace << reinterpret_cast<void*>(static_cast<unsigned char>(char(value))) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, Debug::Flag value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Debug::Flag::value: return debug << "Debug::Flag::" #value;
        _c(NoNewlineAtTheEnd)
        _c(DisableColors)
        _c(NoSpace)
        _c(Packed)
        _c(Color)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Debug::Flag(" << Debug::nospace << reinterpret_cast<void*>(static_cast<unsigned char>(char(value))) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, Debug::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Features{}", {
        Debug::Flag::NoNewlineAtTheEnd,
        Debug::Flag::DisableColors,
        Debug::Flag::NoSpace,
        Debug::Flag::Packed,
        Debug::Flag::Color});
}

/* For some reason Doxygen can't match this with the declaration in DebugStl.h */
Debug& operator<<(Debug& debug, const std::string& value) {
    return debug.print(value);
}
#endif

#ifndef DOXYGEN_GENERATING_OUTPUT
Debug& operator<<(Debug& debug, Implementation::DebugOstreamFallback&& value) {
    return debug.print(value);
}
#endif

}}
