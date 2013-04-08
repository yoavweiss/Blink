The Responsive Images WebKit fork!!!
--------------------

This repository contains experiments in adding the `<picture>` element to
WebKit's WebCore library.

It was last synchronized with WebKit's code at 27/11/12.

*Note*: This is a work in progress. It does not (yet) represent a full
implementation of the `<picture>` element [specification](http://picture.responsiveimages.org/). Please file any
inconsistency with the specification as an issue.


Build instructions:
----------------------
* Build Chromium like you normally would on [Linux](http://code.google.com/p/chromium/wiki/LinuxBuildInstructions), [OSX] (http://code.google.com/p/chromium/wiki/MacBuildInstructions) or [Android] (http://code.google.com/p/chromium/wiki/AndroidBuildInstructions)
* add `"src/third_party/WebKit": "ssh://git@github.com/yoavweiss/webkit.git",` to your `.gclient` `custom_deps` section
* in `src/third_party/WebKit` run `git checkout picture`
* If your time is precious, use [Ninja](http://code.google.com/p/chromium/wiki/NinjaBuild)

Binaries:
--------------------------
* Ubuntu 64bit, OSX & Android binaries can be found at the [downloads]( https://github.com/yoavweiss/webkit/downloads) section.

Test page:
---------------------
Once you got your new binary, you can see `<picture>` in action at the [RICG demos page] (http://demos.responsiveimages.org/)

What should be working:
--------------------
* `<picture><source src></picture>` should work. The *first* source
  should be fetched by the PreloadScanner and then displayed when the
element is parsed.
* `<picture src></picture>` prefetches and displays the resource. It
overrides inner `<source>` tags if present.
* The `media` attribute should be working as expected for both the
PreloadScanner and the actual parser (for `screen` MQs).

What shouldn't be working (yet):
--------------------
* When the viewport is resized, the `<picture>` media queries do not get
  re-evaluated.
* The `srcset` attribute is not yet supported in either the PreloadScanner or the actual parser.
* The `type` attribute is not yet supported in either the PreloadScanner or the actual parser.
* Print media queries are not yet supported.

