// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLSrcsetParser_h
#define HTMLSrcsetParser_h

#include "wtf/text/WTFString.h"

namespace WebCore {

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& sourceSetAttribute);

}

#endif
