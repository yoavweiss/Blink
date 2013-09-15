// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/html/parser/HTMLSrcsetParser.h"

#include "core/html/parser/HTMLParserIdioms.h"
#include "core/platform/text/DecodeEscapeSequences.h"

namespace WebCore {

static inline bool compareByScaleFactor(const ImageCandidate& first, const ImageCandidate& second)
{
    return first.scaleFactor() < second.scaleFactor();
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/embedded-content-1.html#processing-the-image-candidates
void parseImageCandidatesFromSrcsetAttribute(const String& srcsetAttribute, Vector<ImageCandidate>& imageCandidates)
{
    size_t imageCandidateStart = 0;
    unsigned srcsetLength = srcsetAttribute.length();

    while (imageCandidateStart < srcsetLength) {
        float imgScaleFactor = 1.0;
        size_t separator;

        // 4. Splitting loop: Skip whitespace.
        size_t imageUrlStart = srcsetAttribute.find(isNotHTMLSpace, imageCandidateStart);
        if (imageUrlStart == notFound)
            break;
        // If The current candidate is either totally empty or only contains space, skipping.
        if (srcsetAttribute[imageUrlStart] == ',') {
            imageCandidateStart = imageUrlStart + 1;
            continue;
        }
        // 5. Collect a sequence of characters that are not space characters, and let that be url.
        size_t imageUrlEnd = srcsetAttribute.find(isHTMLSpace, imageUrlStart + 1);
        if (imageUrlEnd == notFound) {
            imageUrlEnd = srcsetLength;
            separator = srcsetLength;
        } else if (srcsetAttribute[imageUrlEnd - 1] == ',') {
            --imageUrlEnd;
            separator = imageUrlEnd;
        } else {
            // 7. Collect a sequence of characters that are not "," (U+002C) characters, and let that be descriptors.
            size_t imageScaleStart = srcsetAttribute.find(isNotHTMLSpace, imageUrlEnd + 1);
            if (imageScaleStart == notFound) {
                separator = srcsetLength;
            } else if (srcsetAttribute[imageScaleStart] == ',') {
                separator = imageScaleStart;
            } else {
                // This part differs from the spec as the current implementation only supports pixel density descriptors for now.
                size_t imageScaleEnd = srcsetAttribute.find(isHTMLSpaceOrComma, imageScaleStart + 1);
                imageScaleEnd = (imageScaleEnd == notFound) ? srcsetLength : imageScaleEnd;
                size_t comma = imageScaleEnd;
                // Make sure there are no other descriptors
                while ((comma < srcsetLength - 1) && isHTMLSpace(srcsetAttribute[comma]))
                    ++comma;
                // If the first not html space character after the scale modifier is not a comma,
                // the current candidate is an invalid input.
                if ((comma < srcsetLength - 1) && srcsetAttribute[comma] != ',') {
                    // Find the nearest comma and skip the input.
                    comma = srcsetAttribute.find(',', comma + 1);
                    if (comma == notFound)
                        break;
                    imageCandidateStart = comma + 1;
                    continue;
                }
                separator = comma;
                if (srcsetAttribute[imageScaleEnd - 1] != 'x') {
                    imageCandidateStart = separator + 1;
                    continue;
                }
                bool validScaleFactor = false;
                size_t scaleFactorLengthWithoutUnit = imageScaleEnd - imageScaleStart - 1;
                imgScaleFactor = charactersToFloat(srcsetAttribute.characters8() + imageScaleStart, scaleFactorLengthWithoutUnit, &validScaleFactor);

                if (!validScaleFactor) {
                    imageCandidateStart = separator + 1;
                    continue;
                }
            }
        }

        imageCandidates.append(ImageCandidate(&srcsetAttribute, imageUrlStart, imageUrlEnd - imageUrlStart, imgScaleFactor));
        // 11. Return to the step labeled splitting loop.
        imageCandidateStart = separator + 1;
    }
}

static ImageCandidate pickBestImageCandidate(float deviceScaleFactor, Vector<ImageCandidate>& imageCandidates)
{
    std::stable_sort(imageCandidates.begin(), imageCandidates.end(), compareByScaleFactor);

    size_t i;
    for (i = 0; i < imageCandidates.size() - 1; ++i)
        if (imageCandidates[i].scaleFactor() >= deviceScaleFactor)
            break;
    return imageCandidates[i];
}

ImageCandidate bestFitSourceForSrcsetAttribute(float deviceScaleFactor, const String& srcsetAttribute)
{
    Vector<ImageCandidate> imageCandidates;
    parseImageCandidatesFromSrcsetAttribute(srcsetAttribute, imageCandidates);
    if (imageCandidates.isEmpty())
        return ImageCandidate();
    return pickBestImageCandidate(deviceScaleFactor, imageCandidates);
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& srcsetAttribute)
{
    Vector<ImageCandidate> imageCandidates;

    parseImageCandidatesFromSrcsetAttribute(srcsetAttribute, imageCandidates);

    String src = srcAttribute.simplifyWhiteSpace(isHTMLSpace);
    if (!src.isEmpty())
        imageCandidates.append(ImageCandidate(&srcAttribute, 0, src.length(), 1.0));

    if (imageCandidates.isEmpty())
        return String();

    return pickBestImageCandidate(deviceScaleFactor, imageCandidates).toString();
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, ImageCandidate& srcsetImageCandidate)
{
    Vector<ImageCandidate> imageCandidates;

    if (!srcsetImageCandidate.isEmpty())
        imageCandidates.append(srcsetImageCandidate);

    String src = srcAttribute.simplifyWhiteSpace(isHTMLSpace);
    if (!src.isEmpty())
        imageCandidates.append(ImageCandidate(&srcAttribute, 0, src.length(), 1.0));

    if (imageCandidates.isEmpty())
        return String();

    return pickBestImageCandidate(deviceScaleFactor, imageCandidates).toString();
}

}
