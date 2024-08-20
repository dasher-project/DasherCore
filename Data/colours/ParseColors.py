import glob
import xml.etree.ElementTree as ET

def getAttrib(Element, attrib, alt=None) :
    if(Element == None) : return alt
    return Element.attrib[attrib] if attrib in Element.attrib else alt

def parseColor(element):
    return [int(getAttrib(element, 'r', 0)), int(getAttrib(element, 'g', 0)), int(getAttrib(element, 'b', 0)), int(getAttrib(element, 'a', 255))]

def cleanString(s) : return "".join(x for x in s.lower().replace(" ", ".") if x.isascii() and x.isalnum() or x == ".")

def printColor(color):
    if(len(color) == 3 or (len(color) == 4 and color[3] == 255)):
        return '#{:02x}{:02x}{:02x}'.format(*color)
    if(len(color) == 4):
        return '#{:02x}{:02x}{:02x}{:02x}'.format(*color)

def getColorSequence(colors, range) :
    result = []
    for i in range :
        result.append(printColor(parseColor(colors[i])))
    return ",".join(result)

def addGroup(colorsRange, name, groupColor, generateAltColors, parent, colors):
    group = ET.SubElement(parent, "groupColorInfo")
    group.attrib["name"] = name
    if(groupColor > 0) :
        group.attrib["groupColor"] = printColor(parseColor(colors[groupColor]))
        group.attrib["groupOutlineColor"] = printColor(parseColor(colors[3]))
    group.attrib["nodeColorSequence"] = getColorSequence(colors, colorsRange)
    if(generateAltColors) : group.attrib["altNodeColorSequence"] = getColorSequence(colors, [x + 130 for x in colorsRange])

NamedColors = [
[0, "backgroundColor"],
[1, "inputLineColor"],
[2, "inputPositionColor"],
[5, "crosshairColor"],
[7, "rootNodeColor"],
[3, "defaultOutlineColor"],
[4, "defaultLabelColor"],
[1, "selectionHighlightColor"],
[2, "selectionInactiveColor"],
[2, "circleOutlineColor"],
[242, "circleStoppedColor"],
[241, "circleWaitingColor"],
[240, "circleStartedColor"],
[119, "firstStartBoxColor"],
[120, "secondStartBoxColor"],
[240, "twoPushDynamicActiveMarkerColor"],
[61, "twoPushDynamicInactiveMarkerColor"],
[2, "oneButtonDynamicOuterGuidesColor"],
[62, "twoPushDynamicOuterGuidesColor"],
[0, "infoTextColor"],
[5, "infoTextBackgroundColor"],
[111, "warningTextColor"],
[5, "warningTextBackgroundColor"],
[135, "gameGuideColor"],
[9, "conversionNodeColor"]
]

def cr(min, max) :
    return range(min, max + 1)

knownGroups = [
    ["lowercase", cr(10,39), True, -1],
    ["lowercaseBackground", cr(10,39), True, 99],
    ["uppercase", cr(10,38), True, 111],
    ["punctuation", [105,103,104,100,104], True, 112],
    ["limitedPunctuation", [99,109,105,103,104,100,104], True, 112],
    ["punctuationLong", [90,91,92,93,94,95,96,97,98,99,95,96,97,98,105,106,107,108,109,105,106,107,108,109,106,107,108,109,105,9,100,101,102,103,104,100,104], True, 112], #English with accents, numerals, punctuation
    ["numbers", cr(90,94), True, 113],
    ["accents", [72,82], True, 112],
    ["space", [9], False, -1],
    ["paragraph", [9], False, -1],
    ["paragraphSpace", [9,9], False, -1]
]


def ExtractColors(filename, excludeColors):
    extractedNamedColors = {}
    
    parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
    input = ET.parse(filename, parser=parser).getroot().find("palette")
    paletteName = input.attrib["name"]
    output = ET.Element("colors",{
                "name": paletteName
            })
    if paletteName != "Default" :
        output.attrib[parentName] = "Default"

    colors = input.findall('colour')
    for [colorIndex,colorName] in NamedColors:
        color = printColor(parseColor(colors[colorIndex]))
        if(not colorName in excludeColors or excludeColors[colorName] != color) :
            output.attrib[colorName] = printColor(parseColor(colors[colorIndex]))
            extractedNamedColors[colorName] = printColor(parseColor(colors[colorIndex]))

    for [name, ranges, generateAltColors, groupColor] in knownGroups:
        addGroup(ranges, name, groupColor, generateAltColors, output, colors)
        
    tree = ET.ElementTree(output)
    ET.indent(tree, space="\t", level=0)
    with open(f"../colors/color.{cleanString(paletteName)}.xml", 'wb') as f:
        f.write(b'<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write(b'<!DOCTYPE colors SYSTEM "color.dtd">\n')
        tree.write(f, encoding="utf-8")
    
    return extractedNamedColors
        
defaultNamedColors = ExtractColors("colour.xml", [])
for filename in glob.glob('./colour*.xml'):
    if(filename.endswith("colour.xml")): continue

    ExtractColors(filename, defaultNamedColors)