// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/html/parser/HTMLSrcsetParser.h"

#include "core/html/parser/HTMLParserIdioms.h"
#include "core/platform/text/DecodeEscapeSequences.h"

namespace WebCore {

class ImageWithScale {
public:
    ImageWithScale(const String* source, unsigned start, unsigned length, float scaleFactor)
        : m_string(source->createView(start, length))
        , m_scaleFactor(scaleFactor)
    {
    }

    String toString() const
    {
        if (m_string.is8Bit())
            return String(m_string.characters8(), m_string.length());
        else
            return String(m_string.characters16(), m_string.length());
    }

    float scaleFactor() const
    {
        return m_scaleFactor;
    }

private:
    StringView m_string;
    float m_scaleFactor;
};

static inline bool compareByScaleFactor(const ImageWithScale& first, const ImageWithScale& second)
{
    return first.scaleFactor() < second.scaleFactor();
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/embedded-content-1.html#processing-the-image-candidates
static void parseImagesWithScaleFromSrcSetAttribute(const String& srcSetAttribute, Vector<ImageWithScale>& imageCandidates)
{
    size_t imageCandidateStart = 0;
    unsigned srcSetLength = srcSetAttribute.length();

    while (imageCandidateStart < srcSetLength) {
        float imgScaleFactor = 1.0;
        size_t separator;

        // 4. Splitting loop: Skip whitespace.
        size_t imageUrlStart = srcSetAttribute.find(isNotHTMLSpace, imageCandidateStart);
        if (imageUrlStart == notFound)
            break;
        // If The current candidate is either totally empty or only contains space, skipping.
        if (srcSetAttribute[imageUrlStart] == ',') {
            imageCandidateStart = imageUrlStart + 1;
            continue;
        }
        // 5. Collect a sequence of characters that are not space characters, and let that be url.
        size_t imageUrlEnd = srcSetAttribute.find(isHTMLSpace, imageUrlStart + 1);
        if (imageUrlEnd == notFound) {
            imageUrlEnd = srcSetLength;
            separator = srcSetLength;
        } else if (srcSetAttribute[imageUrlEnd - 1] == ',') {
            --imageUrlEnd;
            separator = imageUrlEnd;
        } else {
            // 7. Collect a sequence of characters that are not "," (U+002C) characters, and let that be descriptors.
            size_t imageScaleStart = srcSetAttribute.find(isNotHTMLSpace, imageUrlEnd + 1);
            if (imageScaleStart == notFound) {
                separator = srcSetLength;
            } else if (srcSetAttribute[imageScaleStart] == ',') {
                separator = imageScaleStart;
            } else {
                // This part differs from the spec as the current implementation only supports pixel density descriptors for now.
                size_t imageScaleEnd = srcSetAttribute.find(isHTMLSpaceOrComma, imageScaleStart + 1);
                imageScaleEnd = (imageScaleEnd == notFound) ? srcSetLength : imageScaleEnd;
                size_t comma = imageScaleEnd;
                // Make sure there are no other descriptors
                while ((comma < srcSetLength - 1) && isHTMLSpace(srcSetAttribute[comma]))
                    ++comma;
                // If the first not html space character after the scale modifier is not a comma,
                // the current candidate is an invalid input.
                if ((comma < srcSetLength - 1) && srcSetAttribute[comma] != ',') {
                    // Find the nearest comma and skip the input.
                    comma = srcSetAttribute.find(',', comma + 1);
                    if (comma == notFound)
                        break;
                    imageCandidateStart = comma + 1;
                    continue;
                }
                separator = comma;
                if (srcSetAttribute[imageScaleEnd - 1] != 'x') {
                    imageCandidateStart = separator + 1;
                    continue;
                }
                bool validScaleFactor = false;
                size_t scaleFactorLengthWithoutUnit = imageScaleEnd - imageScaleStart - 1;
                imgScaleFactor = charactersToFloat(srcSetAttribute.characters8() + imageScaleStart, scaleFactorLengthWithoutUnit, &validScaleFactor);

                if (!validScaleFactor) {
                    imageCandidateStart = separator + 1;
                    continue;
                }
            }
        }

        imageCandidates.append(ImageWithScale(&srcSetAttribute, imageUrlStart, imageUrlEnd - imageUrlStart, imgScaleFactor));
        // 11. Return to the step labeled splitting loop.
        imageCandidateStart = separator + 1;
    }
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& srcSetAttribute)
{
    Vector<ImageWithScale> imageCandidates;

    parseImagesWithScaleFromSrcSetAttribute(srcSetAttribute, imageCandidates);

    String src = srcAttribute.simplifyWhiteSpace(isHTMLSpace);
    if (!src.isEmpty())
        imageCandidates.append(ImageWithScale(&srcAttribute, 0, src.length(), 1.0));

    if (imageCandidates.isEmpty())
        return String();

    std::stable_sort(imageCandidates.begin(), imageCandidates.end(), compareByScaleFactor);

    size_t i;
    for (i = 0; i < imageCandidates.size() - 1; ++i)
        if (imageCandidates[i].scaleFactor() >= deviceScaleFactor)
            break;
    return imageCandidates[i].toString();
}

}
