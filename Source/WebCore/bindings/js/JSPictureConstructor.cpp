/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSPictureConstructor.h"

#include "HTMLPictureElement.h"
#include "HTMLNames.h"
#include "JSHTMLPictureElement.h"
#include "JSNode.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSPictureConstructor);

const ClassInfo JSPictureConstructor::s_info = { "PictureConstructor", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSPictureConstructor) };

JSPictureConstructor::JSPictureConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorWithDocument(structure, globalObject)
{
}

void JSPictureConstructor::finishCreation(ExecState* exec, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(globalObject);
    ASSERT(inherits(&s_info));
    putDirect(exec->globalData(), exec->propertyNames().prototype, JSHTMLPictureElementPrototype::self(exec, globalObject), None);
}

static EncodedJSValue JSC_HOST_CALL constructPicture(ExecState* exec)
{
    JSPictureConstructor* jsConstructor = jsCast<JSPictureConstructor*>(exec->callee());
    Document* document = jsConstructor->document();
    if (!document)
        return throwVMError(exec, createReferenceError(exec, "Picture constructor associated document is unavailable"));

    // Calling toJS on the document causes the JS document wrapper to be
    // added to the window object. This is done to ensure that JSDocument::visit
    // will be called, which will cause the image element to be marked if necessary.
    toJS(exec, jsConstructor->globalObject(), document);
    int width;
    int height;
    int* optionalWidth = 0;
    int* optionalHeight = 0;
    if (exec->argumentCount() > 0) {
        width = exec->argument(0).toInt32(exec);
        optionalWidth = &width;
    }
    if (exec->argumentCount() > 1) {
        height = exec->argument(1).toInt32(exec);
        optionalHeight = &height;
    }

    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(),
        HTMLPictureElement::createForJSConstructor(document, optionalWidth, optionalHeight))));
}

ConstructType JSPictureConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructPicture;
    return ConstructTypeHost;
}

} // namespace WebCore
