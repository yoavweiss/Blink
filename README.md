The Responsive Images Blink fork!!!
--------------------

This branch contains experiments in adding the `<picture>` element to
Blink's core library.

It was last synchronized with Blink's code at 23/04/13.

*Note*: This is a work in progress. It does not (yet) represent a full
implementation of the `<picture>` element [specification](http://picture.responsiveimages.org/). Please file any
inconsistency with the specification as an issue.


Build instructions:
----------------------
* Build Chromium like you normally would on [Linux](http://code.google.com/p/chromium/wiki/LinuxBuildInstructions), [OSX] (http://code.google.com/p/chromium/wiki/MacBuildInstructions) or [Android] (http://code.google.com/p/chromium/wiki/AndroidBuildInstructions)
* run `cd src/third_party/WebKit && git remote add github git@github.com/yoavweiss/Blink.git && git fetch && git checkout -b picture &&  git pull github picture`
* add `"src/third_party/WebKit": "ssh://git@github.com/yoavweiss/webkit.git",` to your `.gclient` `custom_deps` section
* in `src/third_party/WebKit` run `git checkout picture`
* If your time is precious, use [Ninja](http://code.google.com/p/chromium/wiki/NinjaBuild)

Binaries:
--------------------------
Currently I don't have any up-to-date binaries to distribute. But me on
[Twitter](https://twitter.com/yoavweiss) if you *really* want one.

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

