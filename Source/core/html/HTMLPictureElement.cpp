/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#if ENABLE(PICTURE)
#include "HTMLImageElement.h"
#include "HTMLPictureElement.h"

#include "core/css/MediaList.h"
#include "core/css/MediaQueryEvaluator.h"
#include "core/css/StyleResolver.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

HTMLPictureElement::HTMLPictureElement(const QualifiedName& tagName, Document* document)
    : HTMLImageElement(tagName, document, NULL),
      m_hasSrc(false)
{
    ASSERT(hasTagName(pictureTag));
}

PassRefPtr<HTMLPictureElement> HTMLPictureElement::create(Document* document)
{
    return adoptRef(new HTMLPictureElement(imgTag, document));
}

PassRefPtr<HTMLPictureElement> HTMLPictureElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLPictureElement(tagName, document));
}

HTMLPictureElement::~HTMLPictureElement()
{
}

PassRefPtr<HTMLPictureElement> HTMLPictureElement::createForJSConstructor(Document* document, const int* optionalWidth, const int* optionalHeight)
{
    RefPtr<HTMLPictureElement> picture = adoptRef(new HTMLPictureElement(imgTag, document));
    if (optionalWidth)
        picture->setWidth(*optionalWidth);
    if (optionalHeight)
        picture->setHeight(*optionalHeight);
    return picture.release();
}

const Element* HTMLPictureElement::getMatchingSource() const{
    if(!m_hasSrc){
        NodeVector potentialSourceNodes;
        getChildNodes(this, potentialSourceNodes);
        for(    NodeVector::iterator it = potentialSourceNodes.begin();
                it != potentialSourceNodes.end();
                it++){
            RefPtr<Node> nodePtr = *it;
            Node* node = nodePtr.get();
            if (node && (node->hasTagName(sourceTag)) && (node->parentNode() == this)){
                HTMLSourceElement* source = static_cast<HTMLSourceElement*>(node);
                Document* document = documentInternal();
                if (source->fastHasAttribute(srcAttr)) {
                    if (!source->fastHasAttribute(mediaAttr)) {
                        return source;
                    }
                    RefPtr<MediaQuerySet> mediaQueries = MediaQuerySet::createAllowingDescriptionSyntax(source->media());
                    RefPtr<RenderStyle> documentStyle = StyleResolver::styleForDocument(document, 0);
                    MediaQueryEvaluator mediaQueryEvaluator("screen", document->frame(), documentStyle.get());
                    if(mediaQueryEvaluator.eval(mediaQueries.get())){
                        return source;
                    }
                }
            }   
        }   
    }
    return this;
}

void HTMLPictureElement::sourceWasAdded()
{
    updateResources();
}

void HTMLPictureElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if(name == srcAttr)
        m_hasSrc = true;

    HTMLImageElement::parseAttribute(name, value);
}

}
#endif
