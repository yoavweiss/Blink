// [Name] SVGFEComponentTransferElement-svgdom-tableValues-prop.js
// [Expected rendering result] An image with feComponentTransfer filter - and a series of PASS messages

// SVGNumberListToString converts a list to a string.
function SVGNumberListToString(list) {
    var result = "";
    for (var i = 0; i < list.numberOfItems; ++i) {
        // We should multiply and round the value of listItem otherwise the expected value cannot be precisely represented as a floating point 
        // number and later the comparison will fail.
        var item = Math.round(list.getItem(i).value * 1000) / 1000;
        result += item;
        result += " ";
    }
    return result;
}

description("Tests dynamic updates of the 'tableValues' property of the SVGFEComponentTransferElement object")
createSVGTestCase();

var feRFunc = createSVGElement("feFuncR");
feRFunc.setAttribute("type", "table");
feRFunc.setAttribute("tableValues", "0 1 1 0");

var feGFunc = createSVGElement("feFuncG");
feGFunc.setAttribute("type", "table");
feGFunc.setAttribute("tableValues", "0 1 1 0.5");

var feBFunc = createSVGElement("feFuncB");
feBFunc.setAttribute("type", "table");
feBFunc.setAttribute("tableValues", "0 0 1 0.1");

var feAFunc = createSVGElement("feFuncA");
feAFunc.setAttribute("type", "table");
feAFunc.setAttribute("tableValues", "0.5 10 1 0.5");

var feCompnentTransferElement = createSVGElement("feComponentTransfer");
feCompnentTransferElement.appendChild(feRFunc);
feCompnentTransferElement.appendChild(feGFunc);
feCompnentTransferElement.appendChild(feBFunc);
feCompnentTransferElement.appendChild(feAFunc);

var compTranFilter = createSVGElement("filter");
compTranFilter.setAttribute("id", "compTranFilter");
compTranFilter.setAttribute("filterUnits", "objectBoundingBox");
compTranFilter.setAttribute("x", "0%");
compTranFilter.setAttribute("y", "0%");
compTranFilter.setAttribute("width", "100%");
compTranFilter.setAttribute("height", "100%");
compTranFilter.appendChild(feCompnentTransferElement);

var defsElement = createSVGElement("defs");
defsElement.appendChild(compTranFilter);
rootSVGElement.appendChild(defsElement);

var imageElement = createSVGElement("image");
imageElement.setAttributeNS(xlinkNS, "xlink:href", "../W3C-SVG-1.1/resources/struct-image-01.png");
imageElement.setAttribute("width", "400");
imageElement.setAttribute("height", "200");
imageElement.setAttribute("preserveAspectRatio", "none");
imageElement.setAttribute("filter", "url(#compTranFilter)");
rootSVGElement.appendChild(imageElement);

rootSVGElement.setAttribute("width", "400");
rootSVGElement.setAttribute("height", "200");

shouldBeEqualToString("SVGNumberListToString(feRFunc.tableValues.baseVal)", "0 1 1 0 ");
shouldBeEqualToString("SVGNumberListToString(feGFunc.tableValues.baseVal)", "0 1 1 0.5 ");
shouldBeEqualToString("SVGNumberListToString(feBFunc.tableValues.baseVal)", "0 0 1 0.1 ");
shouldBeEqualToString("SVGNumberListToString(feAFunc.tableValues.baseVal)", "0.5 10 1 0.5 ");

function repaintTest() {
    var number1 = rootSVGElement.createSVGNumber();
    number1.value = 0.9;
    feRFunc.tableValues.baseVal.replaceItem(number1, 2);

    var number2 = rootSVGElement.createSVGNumber();
    number2.value = 0.6;
    feGFunc.tableValues.baseVal.replaceItem(number2, 3);

    var number3 = rootSVGElement.createSVGNumber();
    number3.value = 0.2;
    feBFunc.tableValues.baseVal.replaceItem(number3, 3);

    var number4 = rootSVGElement.createSVGNumber();
    number4.value = 0.9;
    feAFunc.tableValues.baseVal.replaceItem(number4, 3);

    shouldBeEqualToString("SVGNumberListToString(feRFunc.tableValues.baseVal)", "0 1 0.9 0 ");
    shouldBeEqualToString("SVGNumberListToString(feGFunc.tableValues.baseVal)", "0 1 1 0.6 ");
    shouldBeEqualToString("SVGNumberListToString(feBFunc.tableValues.baseVal)", "0 0 1 0.2 ");
    shouldBeEqualToString("SVGNumberListToString(feAFunc.tableValues.baseVal)", "0.5 10 1 0.9 ");
}

var successfullyParsed = true;
