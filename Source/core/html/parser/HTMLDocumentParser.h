/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLDocumentParser_h
#define HTMLDocumentParser_h

#include "core/dom/FragmentScriptingPermission.h"
#include "core/dom/ScriptableDocumentParser.h"
#include "core/html/parser/BackgroundHTMLInputStream.h"
#include "core/html/parser/CompactHTMLToken.h"
#include "core/html/parser/HTMLInputStream.h"
#include "core/html/parser/HTMLParserOptions.h"
#include "core/html/parser/HTMLPreloadScanner.h"
#include "core/html/parser/HTMLScriptRunnerHost.h"
#include "core/html/parser/HTMLSourceTracker.h"
#include "core/html/parser/HTMLToken.h"
#include "core/html/parser/HTMLTokenizer.h"
#include "core/html/parser/HTMLTreeBuilderSimulator.h"
#include "core/html/parser/XSSAuditor.h"
#include "core/html/parser/XSSAuditorDelegate.h"
#include "core/loader/cache/CachedResourceClient.h"
#include "core/platform/Timer.h"
#include "core/platform/text/SegmentedString.h"
#include <wtf/Deque.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/TextPosition.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class BackgroundHTMLParser;
class CompactHTMLToken;
class Document;
class DocumentFragment;
class HTMLDocument;
class HTMLParserScheduler;
class HTMLScriptRunner;
class HTMLTreeBuilder;
class HTMLResourcePreloader;
class ScriptController;
class ScriptSourceCode;

class PumpSession;

class HTMLDocumentParser :  public ScriptableDocumentParser, HTMLScriptRunnerHost, CachedResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<HTMLDocumentParser> create(Document* document, bool reportErrors)
    {
        return adoptRef(new HTMLDocumentParser(document, reportErrors));
    }
    virtual ~HTMLDocumentParser();

    // Exposed for HTMLParserScheduler
    void resumeParsingAfterYield();

    static void parseDocumentFragment(const String&, DocumentFragment*, Element* contextElement, ParserContentPolicy = AllowScriptingContent);

    HTMLTokenizer* tokenizer() const { return m_tokenizer.get(); }

    virtual TextPosition textPosition() const;
    virtual OrdinalNumber lineNumber() const;

    virtual void suspendScheduledTasks();
    virtual void resumeScheduledTasks();

    struct ParsedChunk {
        OwnPtr<CompactHTMLTokenStream> tokens;
        PreloadRequestStream preloads;
        XSSInfoStream xssInfos;
        HTMLTokenizer::State tokenizerState;
        HTMLTreeBuilderSimulator::State treeBuilderState;
        HTMLInputCheckpoint inputCheckpoint;
        TokenPreloadScannerCheckpoint preloadScannerCheckpoint;
    };
    void didReceiveParsedChunkFromBackgroundParser(PassOwnPtr<ParsedChunk>);

protected:
    virtual void insert(const SegmentedString&) OVERRIDE;
    virtual void append(PassRefPtr<StringImpl>) OVERRIDE;
    virtual void finish() OVERRIDE;

    HTMLDocumentParser(Document*, bool reportErrors);
    HTMLDocumentParser(DocumentFragment*, Element* contextElement, ParserContentPolicy);

    HTMLTreeBuilder* treeBuilder() const { return m_treeBuilder.get(); }

    void forcePlaintextForTextDocument();

private:
    static PassRefPtr<HTMLDocumentParser> create(DocumentFragment* fragment, Element* contextElement, ParserContentPolicy parserContentPolicy)
    {
        return adoptRef(new HTMLDocumentParser(fragment, contextElement, parserContentPolicy));
    }

    // DocumentParser
    virtual void pinToMainThread() OVERRIDE;
    virtual void detach() OVERRIDE;
    virtual bool hasInsertionPoint() OVERRIDE;
    virtual bool processingData() const OVERRIDE;
    virtual void prepareToStopParsing() OVERRIDE;
    virtual void stopParsing() OVERRIDE;
    virtual bool isWaitingForScripts() const OVERRIDE;
    virtual bool isExecutingScript() const OVERRIDE;
    virtual void executeScriptsWaitingForResources() OVERRIDE;

    // HTMLScriptRunnerHost
    virtual void watchForLoad(CachedResource*) OVERRIDE;
    virtual void stopWatchingForLoad(CachedResource*) OVERRIDE;
    virtual HTMLInputStream& inputStream() { return m_input; }
    virtual bool hasPreloadScanner() const { return m_preloadScanner.get() && !shouldUseThreading(); }
    virtual void appendCurrentInputStreamToPreloadScannerAndScan() OVERRIDE;

    // CachedResourceClient
    virtual void notifyFinished(CachedResource*);

    void startBackgroundParser(PassOwnPtr<MediaValues>);
    void stopBackgroundParser();
    void validateSpeculations(PassOwnPtr<ParsedChunk> lastChunk);
    void discardSpeculationsAndResumeFrom(PassOwnPtr<ParsedChunk> lastChunk, PassOwnPtr<HTMLToken>, PassOwnPtr<HTMLTokenizer>);
    void processParsedChunkFromBackgroundParser(PassOwnPtr<ParsedChunk>);
    void pumpPendingSpeculations();

    Document* contextForParsingSession();

    enum SynchronousMode {
        AllowYield,
        ForceSynchronous,
    };
    bool canTakeNextToken(SynchronousMode, PumpSession&);
    void pumpTokenizer(SynchronousMode);
    void pumpTokenizerIfPossible(SynchronousMode);
    void constructTreeFromHTMLToken(HTMLToken&);
    void constructTreeFromCompactHTMLToken(const CompactHTMLToken&);

    void runScriptsForPausedTreeBuilder();
    void resumeParsingAfterScriptExecution();

    void attemptToEnd();
    void endIfDelayed();
    void attemptToRunDeferredScriptsAndEnd();
    void end();

    bool shouldUseThreading() const { return m_options.useThreading && !m_isPinnedToMainThread; }

    bool isParsingFragment() const;
    bool isScheduledForResume() const;
    bool inPumpSession() const { return m_pumpSessionNestingLevel > 0; }
    bool shouldDelayEnd() const { return inPumpSession() || isWaitingForScripts() || isScheduledForResume() || isExecutingScript(); }

    HTMLToken& token() { return *m_token; }

    HTMLParserOptions m_options;
    HTMLInputStream m_input;

    OwnPtr<HTMLToken> m_token;
    OwnPtr<HTMLTokenizer> m_tokenizer;
    OwnPtr<HTMLScriptRunner> m_scriptRunner;
    OwnPtr<HTMLTreeBuilder> m_treeBuilder;
    OwnPtr<HTMLPreloadScanner> m_preloadScanner;
    OwnPtr<HTMLPreloadScanner> m_insertionPreloadScanner;
    OwnPtr<HTMLParserScheduler> m_parserScheduler;
    HTMLSourceTracker m_sourceTracker;
    TextPosition m_textPosition;
    XSSAuditor m_xssAuditor;
    XSSAuditorDelegate m_xssAuditorDelegate;

    // FIXME: m_lastChunkBeforeScript, m_tokenizer, m_token, and m_input should be combined into a single state object
    // so they can be set and cleared together and passed between threads together.
    OwnPtr<ParsedChunk> m_lastChunkBeforeScript;
    Deque<OwnPtr<ParsedChunk> > m_speculations;
    WeakPtrFactory<HTMLDocumentParser> m_weakFactory;
    WeakPtr<BackgroundHTMLParser> m_backgroundParser;
    OwnPtr<HTMLResourcePreloader> m_preloader;

    bool m_isPinnedToMainThread;
    bool m_endWasDelayed;
    bool m_haveBackgroundParser;
    unsigned m_pumpSessionNestingLevel;
};

}

#endif
