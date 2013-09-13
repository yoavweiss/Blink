// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/html/parser/HTMLParserIdioms.h"
#include "core/html/parser/HTMLSrcsetParser.h"
#include "core/platform/text/DecodeEscapeSequences.h"

namespace WebCore {

struct ImageWithScale {
    String imageURL;
    float scaleFactor;
    bool operator==(const ImageWithScale& image) const
    {
        return scaleFactor == image.scaleFactor && imageURL == image.imageURL;
    }
};

typedef Vector<ImageWithScale> ImageCandidates;

static inline bool compareByScaleFactor(const ImageWithScale& first, const ImageWithScale& second)
{
    return first.scaleFactor < second.scaleFactor;
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& srcSetAttribute)
{
    ImageCandidates imageCandidates;

    String srcSetAttributeValue = srcSetAttribute.simplifyWhiteSpace(isHTMLSpace);
    Vector<String> srcSetTokens;

    srcSetAttributeValue.split(',', srcSetTokens);
    imageCandidates.reserveInitialCapacity(srcSetTokens.size());
    for (size_t i = 0; i < srcSetTokens.size(); ++i) {
        Vector<String> data;
        float imgScaleFactor = 1.0;
        bool validScaleFactor = false;

        srcSetTokens[i].stripWhiteSpace().split(' ', data);
        // There must be at least one candidate descriptor, and the last one must
        // be a scale factor. Since we don't support descriptors other than scale,
        // it's better to discard any rule with such descriptors rather than accept
        // only the scale data.
        if (data.size() != 2)
            continue;
        if (!data.last().endsWith('x'))
            continue;

        imgScaleFactor = data.last().substring(0, data.last().length() - 1).toFloat(&validScaleFactor);
        if (!validScaleFactor)
            continue;

        ImageWithScale image;
        image.imageURL = data[0];
        image.scaleFactor = imgScaleFactor;

        imageCandidates.append(image);
    }

    String src = srcAttribute.simplifyWhiteSpace(isHTMLSpace);
    if (!src.isEmpty()) {
        ImageWithScale image;
        image.imageURL = src;
        image.scaleFactor = 1.0;

        imageCandidates.append(image);
    }

    if (imageCandidates.isEmpty())
        return String();

    std::stable_sort(imageCandidates.begin(), imageCandidates.end(), compareByScaleFactor);

    for (size_t i = 0; i < imageCandidates.size() - 1; ++i) {
        if (imageCandidates[i].scaleFactor >= deviceScaleFactor)
            return decodeEscapeSequences<URLEscapeSequence>(imageCandidates[i].imageURL, UTF8Encoding());
    }
    return decodeEscapeSequences<URLEscapeSequence>(imageCandidates.last().imageURL, UTF8Encoding());
}

}
