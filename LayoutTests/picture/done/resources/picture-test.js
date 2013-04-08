
var picture = null;
var mediaElement = document; // If not set, an event from any element will trigger a waitForEvent() callback.
var console = null;
var printFullTestDetails = true; // This is optionaly switched of by test whose tested values can differ. (see disableFullTestDetailsPrinting())
var Failed = false;

findMediaElement();
logConsole();

if (window.testRunner) {
    // Some track element rendering tests require text pixel dump.
    if (typeof requirePixelDump == "undefined")
        testRunner.dumpAsText();

    testRunner.waitUntilDone();
}

function disableFullTestDetailsPrinting()
{
    printFullTestDetails = false;
}

function enableFullTestDetailsPrinting()
{
    printFullTestDetails = true;
}

function logConsole()
{
    if (!console && document.body) {
        console = document.createElement('div');
        document.body.appendChild(console);
    }
    return console;
}

function findMediaElement()
{
    try {
        picture = document.getElementsByTagName('picture')[0];
        if (picture)
            mediaElement = picture;
    } catch (ex) { }
}

function testAndEnd(testFuncString)
{
    test(testFuncString, true);
}

function test(testFuncString, endit)
{
    logResult(eval(testFuncString), "TEST(" + testFuncString + ")");
    if (endit)
        endTest();
}

function testExpected(testFuncString, expected, comparison)
{
    try {
        var observed = eval(testFuncString);
    } catch (ex) {
        consoleWrite(ex);
        return;
    }

    if (comparison === undefined)
        comparison = '==';

    var success = false;
    switch (comparison)
    {
        case '<':   success = observed <  expected; break;
        case '<=': success = observed <= expected; break;
        case '>':   success = observed >  expected; break;
        case '>=': success = observed >= expected; break;
        case '!=':  success = observed != expected; break;
        case '==': success = observed == expected; break;
        case '===': success = observed === expected; break;
    }

    reportExpected(success, testFuncString, comparison, expected, observed)
}

function testArraysEqual(testFuncString, expected)
{
    var observed;
    try {
        observed = eval(testFuncString);
    } catch (ex) {
        consoleWrite(ex);
        return;
    }

    testExpected(testFuncString + ".length", expected.length);

    for (var i = 0; i < observed.length; i++) {
        testExpected(testFuncString + "[" + i + "]", expected[i]);
    }
}

var testNumber = 0;

function reportExpected(success, testFuncString, comparison, expected, observed)
{
    testNumber++;

    var msg = "Test " + testNumber;

    if (printFullTestDetails || !success)
        msg = "EXPECTED (<em>" + testFuncString + " </em>" + comparison + " '<em>" + expected + "</em>')";

    if (!success)
        msg +=  ", OBSERVED '<em>" + observed + "</em>'";

    logResult(success, msg);
}

function runSilently(testFuncString)
{
    if (printFullTestDetails)
        consoleWrite("RUN(" + testFuncString + ")");
    try {
        eval(testFuncString);
    } catch (ex) {
        if (!printFullTestDetails) {
            // No details were printed previous, give some now.
            // This will be helpful in case of error.
            logResult(Failed, "Error in RUN(" + testFuncString + "):");
        }
        logResult(Failed, "<span style='color:red'>"+ex+"</span>");
    }
}

function run(testFuncString)
{
    consoleWrite("RUN(" + testFuncString + ")");
    try {
        eval(testFuncString);
    } catch (ex) {
        consoleWrite(ex);
    }
}

function waitForEventOnce(eventName, func, endit)
{
    waitForEvent(eventName, func, endit, true)
}

function waitForEventAndEnd(eventName, funcString)
{
    waitForEvent(eventName, funcString, true)
}

function waitForEvent(eventName, func, endit, oneTimeOnly, element)
{
    if (!element)
        element = mediaElement;

    function _eventCallback(event)
    {
        if (oneTimeOnly)
            element.removeEventListener(eventName, _eventCallback, true);

        consoleWrite("EVENT(" + eventName + ")");

        if (func)
            func(event);

        if (endit)
            endTest();
    }

    element.addEventListener(eventName, _eventCallback, true);
}

function waitForEventTestAndEnd(eventName, testFuncString)
{
    waitForEventAndTest(eventName, testFuncString, true);
}

function waitForEventAndFail(eventName)
{
    waitForEventAndTest(eventName, "false", true);
}

function waitForEventAndTest(eventName, testFuncString, endit)
{
    function _eventCallback(event)
    {
        logResult(eval(testFuncString), "EVENT(" + eventName + ") TEST(" + testFuncString + ")");
        if (endit)
            endTest();
    }

    mediaElement.addEventListener(eventName, _eventCallback, true);
}

function testException(testString, exceptionString)
{
    try {
        eval(testString);
    } catch (ex) {
        logResult(ex.code == eval(exceptionString), "TEST(" + testString + ") THROWS("+exceptionString+")");
    }
}

var testEnded = false;

function endTest()
{
    consoleWrite("END OF TEST");
    testEnded = true;
    if (window.testRunner)
        testRunner.notifyDone();
}

function endTestLater()
{
    setTimeout(endTest, 250);
}

function failTestIn(ms)
{
    setTimeout(function () {
        consoleWrite("FAIL: did not end fast enough");
        endTest();
    }, ms);
}

function failTest(text)
{
    logResult(Failed, text);
    endTest();
}


function logResult(success, text)
{
    if (success)
        consoleWrite(text + " <span style='color:green'>OK</span>");
    else
        consoleWrite(text + " <span style='color:red'>FAIL</span>");
}

function consoleWrite(text)
{
    if (testEnded)
        return;
    logConsole().innerHTML += text + "<br>";
}

function relativeURL(url)
{
    return url.substr(url.lastIndexOf('/media/')+7);
}


function allTestsEnded()
{
        endTest();
}

var requiredEvents = [];

function waitForEventsAndCall(eventList, func)
{
    function _eventCallback(event)
    {
        if (!requiredEvents.length)
            return;

        var index = requiredEvents.indexOf(event.type);
        if (index < 0)
            return;

        requiredEvents.splice(index, 1);
        if (requiredEvents.length)
            return;

        func();
    }

    requiredEvents = [];
    for (var i = 0; i < eventList.length; i++) {
        requiredEvents[i] = eventList[i][1];
        eventList[i][0].addEventListener(requiredEvents[i], _eventCallback, true);
    }
}
