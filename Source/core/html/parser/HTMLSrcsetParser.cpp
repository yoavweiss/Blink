// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/html/parser/HTMLSrcsetParser.h"

#include "core/html/parser/HTMLParserIdioms.h"
#include "core/platform/ParsingUtilities.h"
#include "core/platform/Logging.h"

namespace WebCore {

static inline bool compareByScaleFactor(const ImageCandidate& first, const ImageCandidate& second)
{
    return first.scaleFactor() < second.scaleFactor();
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/embedded-content-1.html#processing-the-image-candidates
template<typename CharType>
static void parseImageCandidatesFromSrcsetAttribute(const String& attribute, const CharType* attributeStart, unsigned length, Vector<ImageCandidate>& imageCandidates)
{
    //size_t imageCandidateStart = 0;
    const CharType* position = attributeStart;
    const CharType* attributeEnd = position + length;
    //REMOVE this nullification!!!!!!!!!!!
    String copy = String(attributeStart, length);
    LOG(Media, "Parsing candidates %s", copy.characters8());

    //while (imageCandidateStart < srcsetLength) {
    while (position < attributeEnd) {
        float imgScaleFactor = 1.0;
        const CharType* separator;
        //size_t separator;

        // 4. Splitting loop: Skip whitespace.
        skipWhile<CharType, isHTMLSpace<CharType> >(position, attributeEnd);
        //size_t imageUrlStart = srcsetAttribute.find(isNotHTMLSpace, imageCandidateStart);
        if (position == attributeEnd)
            break;
        //if (imageUrlStart == notFound)
        //    break;
        const CharType* imageUrlStart = position;
        
        // If The current candidate is either totally empty or only contains space, skipping.
        if (*position == ',') {
            ++position;
            continue;
        }
        /*
        if (srcsetAttribute[imageUrlStart] == ',') {
            imageCandidateStart = imageUrlStart + 1;
            continue;
        }
        */
        // 5. Collect a sequence of characters that are not space characters, and let that be url.
        ++position;
        skipWhile<CharType, isNotHTMLSpace<CharType> >(position, attributeEnd);
        const CharType* imageUrlEnd = position;

        String copy2 = String(imageUrlStart, imageUrlEnd - imageUrlStart);
        LOG(Media, "Found URL %s", copy2.characters8());

        //size_t imageUrlEnd = srcsetAttribute.find(isHTMLSpace, imageUrlStart + 1);
        if (position == attributeEnd) {
            separator = position;
        } else if (*(position - 1) == ',') {
            --imageUrlEnd;
            separator = imageUrlEnd;
        /*
            break;
        if (imageUrlEnd == notFound) {
            imageUrlEnd = srcsetLength;
            separator = srcsetLength;
        } else if (srcsetAttribute[imageUrlEnd - 1] == ',') {
            --imageUrlEnd;
            separator = imageUrlEnd;
            */
        } else {
            // 7. Collect a sequence of characters that are not "," (U+002C) characters, and let that be descriptors.
            skipWhile<CharType, isHTMLSpace<CharType> >(position, attributeEnd);
            const CharType* imageScaleStart = position;
            //size_t imageScaleStart = srcsetAttribute.find(isNotHTMLSpace, imageUrlEnd + 1);
            if (position == attributeEnd || *position == ',') {
                separator = position;
                /*

            if (imageScaleStart == notFound) {
                separator = srcsetLength;
            } else if (srcsetAttribute[imageScaleStart] == ',') {
                separator = imageScaleStart;
                */
            } else {
                // This part differs from the spec as the current implementation only supports pixel density descriptors for now.
                skipUntil<CharType, isHTMLSpaceOrComma<CharType> >(position, attributeEnd);
                const CharType* imageScaleEnd = position;
                String copy3 = String(imageScaleStart, imageScaleEnd - imageScaleStart);
                LOG(Media, "Found qualifier %s", copy3.characters8());
                const CharType* comma = position;
                // XX
                //size_t imageScaleEnd = srcsetAttribute.find(isHTMLSpaceOrComma, imageScaleStart + 1);
                //imageScaleEnd = (imageScaleEnd == notFound) ? srcsetLength : imageScaleEnd;
                //size_t comma = imageScaleEnd;
                // Make sure there are no other descriptors
                skipWhile<CharType, isHTMLSpace<CharType> >(comma, attributeEnd);
                //while ((comma < srcsetLength - 1) && isHTMLSpace(srcsetAttribute[comma]))
                //    ++comma;
                // If the first not html space character after the scale modifier is not a comma,
                // the current candidate is an invalid input.
                LOG(Media, "skipped 1 more");
                if (comma != attributeEnd && *comma != ',') {


                LOG(Media, "Not a comma %c", *comma);
                    skipUntil<CharType>(comma, attributeEnd, ',');
                LOG(Media, "skipped Until a comma");
                /*
                    if (comma == attributeEnd) {
                        LOG(Media, "Breaking");
                        break;
                    }
                    */
                    position = comma + 1;
                LOG(Media, "continuing");
                    continue;
                }
                /*
                if ((comma < srcsetLength - 1) && srcsetAttribute[comma] != ',') {
                    // Find the nearest comma and skip the input.
                    comma = srcsetAttribute.find(',', comma + 1);
                    if (comma == notFound)
                        break;
                    imageCandidateStart = comma + 1;
                    continue;
                }
                */
                separator = comma;
                if (*(imageScaleEnd - 1) != 'x') {
                    position = separator + 1;
                    LOG(Media, "Not an x qualifier");
                    continue;
                }
                /*
                if (srcsetAttribute[imageScaleEnd - 1] != 'x') {
                    imageCandidateStart = separator + 1;
                    continue;
                }
                */
                bool validScaleFactor = false;
                unsigned scaleFactorLengthWithoutUnit = imageScaleEnd - imageScaleStart - 1;
                LOG(Media, "Gonna convert to float");
                imgScaleFactor = charactersToFloat(imageScaleStart, scaleFactorLengthWithoutUnit, &validScaleFactor);

                LOG(Media, "scale factor %f", imgScaleFactor);

                if (!validScaleFactor) {
                    position = separator + 1;
                    continue;
                }
            }
        }

        String copy4 = String(imageUrlStart, imageUrlEnd - imageUrlStart);
        LOG(Media, "Found qualifier %s", copy4.characters8());

        imageCandidates.append(ImageCandidate(attribute, imageUrlStart - attributeStart, imageUrlEnd - imageUrlStart, imgScaleFactor));
        // 11. Return to the step labeled splitting loop.
        position = separator + 1;
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
    unsigned srcsetLength = srcsetAttribute.length();
    if (srcsetAttribute.is8Bit())
        parseImageCandidatesFromSrcsetAttribute<LChar>(srcsetAttribute, srcsetAttribute.characters8(), srcsetLength, imageCandidates);
    else
        parseImageCandidatesFromSrcsetAttribute<UChar>(srcsetAttribute, srcsetAttribute.characters16(), srcsetLength, imageCandidates);
    if (imageCandidates.isEmpty())
        return ImageCandidate();
    return pickBestImageCandidate(deviceScaleFactor, imageCandidates);
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& srcsetAttribute)
{
    if (srcsetAttribute.isNull())
        return srcAttribute;

    Vector<ImageCandidate> imageCandidates;

    unsigned srcsetLength = srcsetAttribute.length();
    if (srcsetAttribute.is8Bit())
        parseImageCandidatesFromSrcsetAttribute<LChar>(srcsetAttribute, srcsetAttribute.characters8(), srcsetLength, imageCandidates);
    else
        parseImageCandidatesFromSrcsetAttribute<UChar>(srcsetAttribute, srcsetAttribute.characters16(), srcsetLength, imageCandidates);

    String src = srcAttribute.simplifyWhiteSpace(isHTMLSpace);
    if (!src.isEmpty())
        imageCandidates.append(ImageCandidate(srcAttribute, 0, srcAttribute.length(), 1.0));

    if (imageCandidates.isEmpty())
        return String();

    return pickBestImageCandidate(deviceScaleFactor, imageCandidates).toString();
}

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, ImageCandidate& srcsetImageCandidate)
{
    if (srcsetImageCandidate.isEmpty())
        return srcAttribute;

    Vector<ImageCandidate> imageCandidates;
    imageCandidates.append(srcsetImageCandidate);

    String src = srcAttribute.simplifyWhiteSpace(isHTMLSpace);
    if (!src.isEmpty()) {
        imageCandidates.append(ImageCandidate(srcAttribute, 0, srcAttribute.length(), 1.0));
    }

    return pickBestImageCandidate(deviceScaleFactor, imageCandidates).toString();
}

}
