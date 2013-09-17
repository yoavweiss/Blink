// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ParsingUtilities_h
#define ParsingUtilities_h

template<typename CharType>
static bool skipExactly(const CharType*& position, const CharType* end, CharType delimiter)
{
    if (position < end && *position == delimiter) {
        ++position;
        return true;
    }
    return false;
}

template<typename CharType, bool characterPredicate(CharType)>
static bool skipExactly(const CharType*& position, const CharType* end)
{
    if (position < end && characterPredicate(*position)) {
        ++position;
        return true;
    }
    return false;
}

template<typename CharType>
static void skipUntil(const CharType*& position, const CharType* end, CharType delimiter)
{
    while (position < end && *position != delimiter)
        ++position;
}

template<typename CharType, bool characterPredicate(CharType)>
static void skipUntil(const CharType*& position, const CharType* end)
{
    while (position < end && !characterPredicate(*position))
        ++position;
}

template<typename CharType, bool characterPredicate(CharType)>
static void skipWhile(const CharType*& position, const CharType* end)
{
    while (position < end && characterPredicate(*position))
        ++position;
}

#endif

