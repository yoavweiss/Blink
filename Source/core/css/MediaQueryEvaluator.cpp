/*
 * CSS Media Query Evaluator
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/css/MediaQueryEvaluator.h"

#include "CSSValueKeywords.h"
#include "core/css/CSSAspectRatioValue.h"
#include "core/css/CSSHelper.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/MediaFeatureNames.h"
#include "core/css/MediaList.h"
#include "core/css/MediaQuery.h"
#include "core/css/MediaQueryExp.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/dom/NodeRenderStyle.h"
#include "core/dom/Document.h"
#include "core/page/Frame.h"
#include "core/page/FrameView.h"
#include "core/page/Page.h"
#include "core/page/Settings.h"
#include "core/platform/PlatformScreen.h"
#include "core/platform/graphics/FloatRect.h"
#include "core/rendering/RenderLayerCompositor.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/style/RenderStyle.h"
#include "wtf/HashMap.h"

namespace WebCore {

using namespace MediaFeatureNames;

enum MediaFeaturePrefix { MinPrefix, MaxPrefix, NoPrefix };

typedef bool (*EvalFunc)(CSSValue*, RenderStyle*, Frame*, MediaFeaturePrefix, MediaValues*, bool);
typedef HashMap<StringImpl*, EvalFunc> FunctionMap;
static FunctionMap* gFunctionMap;

MediaQueryEvaluator::MediaQueryEvaluator(bool mediaFeatureResult)
    : m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
    , m_mediaValues(0)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const String& acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
    , m_mediaValues(0)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const char* acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
    , m_mediaValues(0)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const String& acceptedMediaType, Frame* frame, RenderStyle* style)
    : m_mediaType(acceptedMediaType)
    , m_frame(frame)
    , m_style(style)
    , m_expResult(false) // doesn't matter when we have m_frame and m_style
    , m_mediaValues(0)
{
}

MediaQueryEvaluator::MediaQueryEvaluator(const String& acceptedMediaType, const MediaValues* mediaValues, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
    , m_mediaValues(MediaValues::copy(mediaValues))
{
}

MediaQueryEvaluator::~MediaQueryEvaluator()
{
}

bool MediaQueryEvaluator::mediaTypeMatch(const String& mediaTypeToMatch) const
{
    return mediaTypeToMatch.isEmpty()
        || equalIgnoringCase(mediaTypeToMatch, "all")
        || equalIgnoringCase(mediaTypeToMatch, m_mediaType);
}

bool MediaQueryEvaluator::mediaTypeMatchSpecific(const char* mediaTypeToMatch) const
{
    // Like mediaTypeMatch, but without the special cases for "" and "all".
    ASSERT(mediaTypeToMatch);
    ASSERT(mediaTypeToMatch[0] != '\0');
    ASSERT(!equalIgnoringCase(mediaTypeToMatch, String("all")));
    return equalIgnoringCase(mediaTypeToMatch, m_mediaType);
}

static bool applyRestrictor(MediaQuery::Restrictor r, bool value)
{
    return r == MediaQuery::Not ? !value : value;
}

bool MediaQueryEvaluator::eval(const MediaQuerySet* querySet, StyleResolver* styleResolver) const
{
    if (!querySet)
        return true;

    const Vector<OwnPtr<MediaQuery> >& queries = querySet->queryVector();
    if (!queries.size())
        return true; // empty query list evaluates to true

    // iterate over queries, stop if any of them eval to true (OR semantics)
    bool result = false;
    for (size_t i = 0; i < queries.size() && !result; ++i) {
        MediaQuery* query = queries[i].get();

        if (mediaTypeMatch(query->mediaType())) {
            const Vector<OwnPtr<MediaQueryExp> >* exps = query->expressions();
            // iterate through expressions, stop if any of them eval to false
            // (AND semantics)
            size_t j = 0;
            for (; j < exps->size(); ++j) {
                bool exprResult = eval(exps->at(j).get());
                if (styleResolver && exps->at(j)->isViewportDependent())
                    styleResolver->addViewportDependentMediaQueryResult(exps->at(j).get(), exprResult);
                if (!exprResult)
                    break;
            }

            // assume true if we are at the end of the list,
            // otherwise assume false
            result = applyRestrictor(query->restrictor(), exps->size() == j);
        } else {
            result = applyRestrictor(query->restrictor(), false);
        }
    }

    return result;
}

template<typename T>
bool compareValue(T a, T b, MediaFeaturePrefix op)
{
    switch (op) {
    case MinPrefix:
        return a >= b;
    case MaxPrefix:
        return a <= b;
    case NoPrefix:
        return a == b;
    }
    return false;
}

static bool compareAspectRatioValue(CSSValue* value, int width, int height, MediaFeaturePrefix op)
{
    if (value->isAspectRatioValue()) {
        CSSAspectRatioValue* aspectRatio = static_cast<CSSAspectRatioValue*>(value);
        return compareValue(width * static_cast<int>(aspectRatio->denominatorValue()), height * static_cast<int>(aspectRatio->numeratorValue()), op);
    }

    return false;
}

static bool numberValue(CSSValue* value, float& result)
{
    if (value->isPrimitiveValue()
        && toCSSPrimitiveValue(value)->isNumber()) {
        result = toCSSPrimitiveValue(value)->getFloatValue(CSSPrimitiveValue::CSS_NUMBER);
        return true;
    }
    return false;
}

static bool colorMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    int bitsPerComponent;
    if (frame)
        bitsPerComponent = screenDepthPerComponent(frame->page()->mainFrame()->view());
    else
        bitsPerComponent = mediaValues->getColorBitsPerComponent();

    float number;
    if (value)
        return numberValue(value, number) && compareValue(bitsPerComponent, static_cast<int>(number), op);

    return bitsPerComponent != 0;
}

static bool colorIndexMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op, MediaValues*, bool)
{
    // FIXME: We currently assume that we do not support indexed displays, as it is unknown
    // how to retrieve the information if the display mode is indexed. This matches Firefox.
    if (!value)
        return false;

    float number;
    return numberValue(value, number) && compareValue(0, static_cast<int>(number), op);
}

static bool monochromeMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if ((!frame || !style) && !mediaValues)
        return expectedValue;

    bool isMonochrome;
    if (frame)
        isMonochrome = screenIsMonochrome(frame->page()->mainFrame()->view());
    else
        isMonochrome = (mediaValues->getMonochromeBitsPerComponent() > 0);

    if (!isMonochrome) {
        if (value) {
            float number;
            return numberValue(value, number) && compareValue(0, static_cast<int>(number), op);
        }
        return false;
    }

    return colorMediaFeatureEval(value, style, frame, op, mediaValues, expectedValue);
}

static IntSize viewportSize(FrameView* view)
{
    return view->layoutSize(ScrollableArea::IncludeScrollbars);
}

static bool orientationMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    int width;
    int height;
    if (frame) {
        FrameView* view = frame->view();
        width = viewportSize(view).width();
        height = viewportSize(view).height();
    } else {
        width = mediaValues->getViewportWidth();
        height = mediaValues->getViewportHeight();
    }

    if (value && value->isPrimitiveValue()) {
        const CSSValueID id = toCSSPrimitiveValue(value)->getValueID();
        if (width > height) // Square viewport is portrait.
            return CSSValueLandscape == id;
        return CSSValuePortrait == id;
    }

    // Expression (orientation) evaluates to true if width and height >= 0.
    return height >= 0 && width >= 0;
}

static bool aspectRatioMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    if (value) {
        int width;
        int height;
        if (frame) {
            FrameView* view = frame->view();
            width = viewportSize(view).width();
            height = viewportSize(view).height();
        } else {
            width = mediaValues->getViewportWidth();
            height = mediaValues->getViewportHeight();
        }

        return compareAspectRatioValue(value, width, height, op);
    }

    // ({,min-,max-}aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero
    return true;
}

static bool deviceAspectRatioMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    if (value) {
        int width;
        int height;
        if (frame) {
            FloatRect sg = screenRect(frame->page()->mainFrame()->view());
            width = static_cast<int>(sg.width());
            height = static_cast<int>(sg.height());
        } else {
            width = mediaValues->getDeviceWidth();
            height = mediaValues->getDeviceHeight();
        }

        return compareAspectRatioValue(value, width, height, op);
    }

    // ({,min-,max-}device-aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero
    return true;
}

static bool evalResolution(CSSValue* value, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    // FIXME: Possible handle other media types than 'screen' and 'print'.
    float deviceScaleFactor = 0;

    // This checks the actual media type applied to the document, and we know
    // this method only got called if this media type matches the one defined
    // in the query. Thus, if if the document's media type is "print", the
    // media type of the query will either be "print" or "all".
    if (frame) {
        String mediaType = frame->view()->mediaType();
        if (equalIgnoringCase(mediaType, "screen")) {
            deviceScaleFactor = frame->page()->deviceScaleFactor();
        } else if (equalIgnoringCase(mediaType, "print")) {
            // The resolution of images while printing should not depend on the DPI
            // of the screen. Until we support proper ways of querying this info
            // we use 300px which is considered minimum for current printers.
            deviceScaleFactor = 300 / cssPixelsPerInch;
        }
    } else {
        deviceScaleFactor = mediaValues->getPixelRatio();
    }

    if (!value)
        return !!deviceScaleFactor;

    if (!value->isPrimitiveValue())
        return false;

    CSSPrimitiveValue* resolution = toCSSPrimitiveValue(value);

    if (resolution->isNumber())
        return compareValue(deviceScaleFactor, resolution->getFloatValue(), op);

    if (!resolution->isResolution())
        return false;

    if (resolution->isDotsPerCentimeter()) {
        // To match DPCM to DPPX values, we limit to 2 decimal points.
        // The http://dev.w3.org/csswg/css3-values/#absolute-lengths recommends
        // "that the pixel unit refer to the whole number of device pixels that best
        // approximates the reference pixel". With that in mind, allowing 2 decimal
        // point precision seems appropriate.
        return compareValue(
            floorf(0.5 + 100 * deviceScaleFactor) / 100,
            floorf(0.5 + 100 * resolution->getFloatValue(CSSPrimitiveValue::CSS_DPPX)) / 100, op);
    }

    return compareValue(deviceScaleFactor, resolution->getFloatValue(CSSPrimitiveValue::CSS_DPPX), op);
}

static bool devicePixelRatioMediaFeatureEval(CSSValue *value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    return (!value || toCSSPrimitiveValue(value)->isNumber()) && evalResolution(value, frame, op, mediaValues, expectedValue);
}

static bool resolutionMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues* MediaValues, bool expectedValue)
{
    return (!value || toCSSPrimitiveValue(value)->isResolution()) && evalResolution(value, frame, op, MediaValues, expectedValue);
}

static bool gridMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op, MediaValues*, bool)
{
    // if output device is bitmap, grid: 0 == true
    // assume we have bitmap device
    float number;
    if (value && numberValue(value, number))
        return compareValue(static_cast<int>(number), 0, op);
    return false;
}

static bool computeLength(CSSValue* value, bool strict, RenderStyle* style, RenderStyle* rootStyle, int defaultFontSize, int& result)
{
    if (!value->isPrimitiveValue())
        return false;

    CSSPrimitiveValue* primitiveValue = toCSSPrimitiveValue(value);

    if (primitiveValue->isNumber()) {
        result = primitiveValue->getIntValue();
        return !strict || !result;
    }

    if (primitiveValue->isLength()) {
        if (style) {
            result = primitiveValue->computeLength<int>(style, rootStyle, 1.0 /* multiplier */, true /* computingFontSize */);
        } else {
            unsigned short type = primitiveValue->primitiveType();
            int factor = 0;
            if (type == CSSPrimitiveValue::CSS_EMS || type == CSSPrimitiveValue::CSS_REMS) {
                if(defaultFontSize>0)
                    factor = defaultFontSize;
                else
                    return false;
            } else if (type == CSSPrimitiveValue::CSS_PX) {
                factor = 1;
            } else {
                return false;
            }
            result = roundForImpreciseConversion<int>(primitiveValue->getDoubleValue()*factor);
        }
        return true;
    }

    return false;
}

static bool deviceHeightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if ((!frame || !style) && !mediaValues)
        return expectedValue;
    int fontSize = 0;
    if(mediaValues)
        fontSize = mediaValues->getDefaultFontSize();

    if (value) {
        RenderStyle* rootStyle;
        long height;
        int length;
        bool inStrictMode = true;
        if (frame) {
            FloatRect sg = screenRect(frame->page()->mainFrame()->view());
            rootStyle = frame->document()->documentElement()->renderStyle();
            height = sg.height();
            inStrictMode = !frame->document()->inQuirksMode();
        } else {
            height = mediaValues->getDeviceHeight();
        }
        InspectorInstrumentation::applyScreenHeightOverride(frame, &height);
        return computeLength(value, inStrictMode, style, rootStyle, fontSize, length) && compareValue(static_cast<int>(height), length, op);
    }
    // ({,min-,max-}device-height)
    // assume if we have a device, assume non-zero
    return true;
}

static bool deviceWidthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if ((!frame || !style) && !mediaValues)
        return expectedValue;
    int fontSize = 0;
    if(mediaValues)
        fontSize = mediaValues->getDefaultFontSize();

    if (value) {
        RenderStyle* rootStyle;
        long width;
        int length;
        bool inStrictMode = true;
        if (frame) {
            FloatRect sg = screenRect(frame->page()->mainFrame()->view());
            rootStyle = frame->document()->documentElement()->renderStyle();
            width = sg.width();
            inStrictMode = !frame->document()->inQuirksMode();
        } else {
            width = mediaValues->getDeviceWidth();
        }
        InspectorInstrumentation::applyScreenWidthOverride(frame, &width);
        return computeLength(value, inStrictMode, style, rootStyle, fontSize, length) && compareValue(static_cast<int>(width), length, op);
    }
    // ({,min-,max-}device-width)
    // assume if we have a device, assume non-zero
    return true;
}

static bool heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if ((!frame || !style) && !mediaValues)
        return expectedValue;
    int fontSize = 0;
    if(mediaValues)
        fontSize = mediaValues->getDefaultFontSize();

    FrameView* view = frame->view();

    int height = viewportSize(view).height();
    if (value) {
        RenderView* renderView = 0;
        RenderStyle* rootStyle = 0;
        bool inStrictMode = true;
        if (frame) {
            if (renderView = frame->document()->renderView())
                height = adjustForAbsoluteZoom(height, renderView);
            rootStyle = frame->document()->documentElement()->renderStyle();
            inStrictMode = !frame->document()->inQuirksMode();
        }
        int length;
        return computeLength(value, inStrictMode, style, rootStyle, fontSize, length) && compareValue(height, length, op);
    }

    return height;
}

static bool widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    if ((!frame || !style) && !mediaValues)
        return expectedValue;
    int fontSize = 0;
    int width;
    if(mediaValues)
        fontSize = mediaValues->getDefaultFontSize();

    if(frame){
        FrameView* view = frame->view();
        width = viewportSize(view).width();
    } else {
        width = mediaValues->getViewportWidth();
    }

    if (value) {
        RenderView* renderView = 0;
        RenderStyle* rootStyle = 0;
        bool inStrictMode = true;
        if (frame) {
            if (renderView = frame->document()->renderView())
                width = adjustForAbsoluteZoom(width, renderView);
            rootStyle = frame->document()->documentElement()->renderStyle();
            inStrictMode = !frame->document()->inQuirksMode();
        }
        int length;
        return computeLength(value, inStrictMode, style, rootStyle, fontSize, length) && compareValue(width, length, op);
    }

    return width;
}

// rest of the functions are trampolines which set the prefix according to the media feature expression used

static bool minColorMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return colorMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxColorMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return colorMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minColorIndexMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues*, bool expectedValue)
{
    return colorIndexMediaFeatureEval(value, style, frame, MinPrefix, 0, expectedValue);
}

static bool maxColorIndexMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues*, bool expectedValue)
{
    return colorIndexMediaFeatureEval(value, style, frame, MaxPrefix, 0, expectedValue);
}

static bool minMonochromeMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return monochromeMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxMonochromeMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return monochromeMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minAspectRatioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return aspectRatioMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxAspectRatioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return aspectRatioMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minDeviceAspectRatioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return deviceAspectRatioMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxDeviceAspectRatioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return deviceAspectRatioMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minDevicePixelRatioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return devicePixelRatioMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxDevicePixelRatioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return devicePixelRatioMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minHeightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return heightMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxHeightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return heightMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minWidthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return widthMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxWidthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return widthMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minDeviceHeightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return deviceHeightMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxDeviceHeightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return deviceHeightMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minDeviceWidthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return deviceWidthMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxDeviceWidthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return deviceWidthMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool minResolutionMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return resolutionMediaFeatureEval(value, style, frame, MinPrefix, mediaValues, expectedValue);
}

static bool maxResolutionMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    return resolutionMediaFeatureEval(value, style, frame, MaxPrefix, mediaValues, expectedValue);
}

static bool animationMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op, MediaValues*, bool)
{
    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool deprecatedTransitionMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues*, bool)
{
    UseCounter::countDeprecation(frame->document(), UseCounter::PrefixedTransitionMediaFeature);

    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transform2dMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op, MediaValues*, bool)
{
    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transform3dMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues* mediaValues, bool expectedValue)
{
    // TODO: Not implementing mediaValues yet
    if (!frame && !mediaValues)
        return expectedValue;

    bool returnValueIfNoParameter;
    int have3dRendering;

    bool threeDEnabled = false;
    if (frame) {
        if (RenderView* view = frame->contentRenderer())
            threeDEnabled = view->compositor()->canRender3DTransforms();
    } else {
        threeDEnabled = mediaValues->getThreeDEnabled();
    }

    returnValueIfNoParameter = threeDEnabled;
    have3dRendering = threeDEnabled ? 1 : 0;

    if (value) {
        float number;
        return numberValue(value, number) && compareValue(have3dRendering, static_cast<int>(number), op);
    }
    return returnValueIfNoParameter;
}

static bool viewModeMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op, MediaValues*, bool)
{
    UNUSED_PARAM(op);
    if (!value)
        return true;

    return toCSSPrimitiveValue(value)->getValueID() == CSSValueWindowed;
}

enum PointerDeviceType { TouchPointer, MousePointer, NoPointer, UnknownPointer };

static PointerDeviceType leastCapablePrimaryPointerDeviceType(Frame* frame)
{
    if (frame->settings()->deviceSupportsTouch())
        return TouchPointer;

    // FIXME: We should also try to determine if we know we have a mouse.
    // When we do this, we'll also need to differentiate between known not to
    // have mouse or touch screen (NoPointer) and unknown (UnknownPointer).
    // We could also take into account other preferences like accessibility
    // settings to decide which of the available pointers should be considered
    // "primary".

    return UnknownPointer;
}

static bool hoverMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    PointerDeviceType pointer;
    if (frame)
        pointer = leastCapablePrimaryPointerDeviceType(frame);
    else
        pointer = static_cast<PointerDeviceType>(mediaValues->getPointer());

    // If we're on a port that hasn't explicitly opted into providing pointer device information
    // (or otherwise can't be confident in the pointer hardware available), then behave exactly
    // as if this feature feature isn't supported.
    if (pointer == UnknownPointer)
        return false;

    float number = 1;
    if (value) {
        if (!numberValue(value, number))
            return false;
    }

    return (pointer == NoPointer && !number)
        || (pointer == TouchPointer && !number)
        || (pointer == MousePointer && number == 1);
}

static bool pointerMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    PointerDeviceType pointer;
    if (frame)
        pointer = leastCapablePrimaryPointerDeviceType(frame);
    else
        pointer = static_cast<PointerDeviceType>(mediaValues->getPointer());

    // If we're on a port that hasn't explicitly opted into providing pointer device information
    // (or otherwise can't be confident in the pointer hardware available), then behave exactly
    // as if this feature feature isn't supported.
    if (pointer == UnknownPointer)
        return false;

    if (!value)
        return pointer != NoPointer;

    if (!value->isPrimitiveValue())
        return false;

    const CSSValueID id = toCSSPrimitiveValue(value)->getValueID();
    return (pointer == NoPointer && id == CSSValueNone)
        || (pointer == TouchPointer && id == CSSValueCoarse)
        || (pointer == MousePointer && id == CSSValueFine);
}

static bool scanMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix, MediaValues* mediaValues, bool expectedValue)
{
    if (!frame && !mediaValues)
        return expectedValue;

    // Scan only applies to tv media.
    String mediaType;
    if (frame)
        mediaType = frame->view()->mediaType();
    else
        mediaType = mediaValues->getMediaType();

    if (!equalIgnoringCase(frame->view()->mediaType(), "tv"))
        return false;

    if (!value)
        return true;

    if (!value->isPrimitiveValue())
        return false;

    // If a platform interface supplies progressive/interlace info for TVs in the
    // future, it needs to be handled here. For now, assume a modern TV with
    // progressive display.
    return toCSSPrimitiveValue(value)->getValueID() == CSSValueProgressive;
}

static void createFunctionMap()
{
    // Create the table.
    gFunctionMap = new FunctionMap;
#define ADD_TO_FUNCTIONMAP(name, str)  \
    gFunctionMap->set(name##MediaFeature.impl(), name##MediaFeatureEval);
    CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(ADD_TO_FUNCTIONMAP);
#undef ADD_TO_FUNCTIONMAP
}

bool MediaQueryEvaluator::eval(const MediaQueryExp* expr) const
{
    if (!m_mediaValues && (!m_frame || !m_style))
        return m_expResult;

    if (!gFunctionMap)
        createFunctionMap();

    // call the media feature evaluation function. Assume no prefix
    // and let trampoline functions override the prefix if prefix is
    // used
    EvalFunc func = gFunctionMap->get(expr->mediaFeature().impl());
    if (func)
        return func(expr->value(), m_style.get(), m_frame, NoPrefix, m_mediaValues.get(), m_expResult);

    return false;
}

PassRefPtr<MediaValues> MediaValues::create(Document* document) {
    ASSERT(document->frame());
    ASSERT(document->renderer());
    ASSERT(document->renderer()->style());
    Frame* frame = document->frame();
    RenderStyle* style = document->renderer()->style();

    // get the values from Frame and Style
    FrameView* view = frame->view();
    RenderView* renderView = frame->document()->renderView();
    int viewportWidth = viewportSize(view).width();
    int viewportHeight = viewportSize(view).height();
    FloatRect sg = screenRect(frame->page()->mainFrame()->view());
    int deviceWidth = static_cast<int>(sg.width());
    int deviceHeight = static_cast<int>(sg.height());
    float pixelRatio = frame->page()->deviceScaleFactor();
    int bitsPerComponent = screenDepthPerComponent(frame->page()->mainFrame()->view());
    int colorBitsPerComponent = 0;
    int monochromeBitsPerComponent = 0;
    int defaultFontSize = style->fontDescription().specifiedSize();
    bool threeDEnabled = false;
    String mediaType = frame->view()->mediaType();
    if (renderView) {
        viewportWidth = adjustForAbsoluteZoom(viewportWidth, renderView);
        viewportHeight = adjustForAbsoluteZoom(viewportHeight, renderView);
    }
    if (RenderView* view = frame->contentRenderer())
        threeDEnabled = view->compositor()->canRender3DTransforms();
    if (screenIsMonochrome(frame->page()->mainFrame()->view()))
        monochromeBitsPerComponent = bitsPerComponent;
    else
        colorBitsPerComponent = bitsPerComponent;
    PointerDeviceType pointer = leastCapablePrimaryPointerDeviceType(frame);

    return adoptRef(new MediaValues(viewportWidth,
        viewportHeight,
        deviceWidth,
        deviceHeight,
        pixelRatio,
        colorBitsPerComponent,
        monochromeBitsPerComponent,
        pointer,
        defaultFontSize,
        threeDEnabled,
        mediaType));
}
PassRefPtr<MediaValues> MediaValues::copy(const MediaValues* mediaValues) {
    return adoptRef(new MediaValues(mediaValues->m_viewportWidth,
        mediaValues->m_viewportHeight,
        mediaValues->m_deviceWidth,
        mediaValues->m_deviceHeight,
        mediaValues->m_pixelRatio,
        mediaValues->m_colorBitsPerComponent,
        mediaValues->m_monochromeBitsPerComponent,
        mediaValues->m_pointer,
        mediaValues->m_defaultFontSize,
        mediaValues->m_threeDEnabled,
        mediaValues->m_mediaType));
}

} // namespace
