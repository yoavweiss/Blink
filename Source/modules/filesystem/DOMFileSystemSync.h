/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DOMFileSystemSync_h
#define DOMFileSystemSync_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "modules/filesystem/DOMFileSystemBase.h"
#include "platform/heap/Handle.h"

namespace blink {

class DirectoryEntrySync;
class File;
class FileEntrySync;
class FileWriterSync;
class ExceptionState;

class DOMFileSystemSync FINAL : public DOMFileSystemBase, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static DOMFileSystemSync* create(ExecutionContext* context, const String& name, FileSystemType type, const KURL& rootURL)
    {
        return new DOMFileSystemSync(context, name, type, rootURL);
    }

    static DOMFileSystemSync* create(DOMFileSystemBase*);

    virtual ~DOMFileSystemSync();

    virtual void reportError(ErrorCallback*, FileError*) OVERRIDE;

    DirectoryEntrySync* root();

    File* createFile(const FileEntrySync*, ExceptionState&);
    FileWriterSync* createWriter(const FileEntrySync*, ExceptionState&);

private:
    DOMFileSystemSync(ExecutionContext*, const String& name, FileSystemType, const KURL& rootURL);
};

} // namespace blink

#endif // DOMFileSystemSync_h
