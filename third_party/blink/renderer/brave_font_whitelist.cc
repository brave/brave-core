/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/brave_font_whitelist.h"

#include <vector>

namespace brave {

namespace {

base::flat_set<base::StringPiece> kEmptyFontSet =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});

#if BUILDFLAG(IS_MAC)
bool kCanRestrictFonts = true;
// This list covers the fonts installed by default on Mac OS as of Mac OS 12.3.
base::flat_set<base::StringPiece> kFontWhitelist =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "-apple-system",
        "academy engraved let",
        "al bayan",
        "al nile",
        "al tarikh",
        "american typewriter",
        "andale mono",
        "apple braille outline 6 dot",
        "apple braille outline 8 dot",
        "apple braille pinpoint 6 dot",
        "apple braille pinpoint 8 dot",
        "apple braille",
        "apple chancery",
        "apple color emoji",
        "apple sd gothic neo",
        "apple symbols",
        "applegothic",
        "applemyungjo",
        "aquakana",
        "arial black",
        "arial hebrew scholar",
        "arial hebrew",
        "arial narrow",
        "arial rounded mt bold",
        "arial unicode ms",
        "arial",
        "athelas",
        "avenir black oblique",
        "avenir black",
        "avenir book",
        "avenir heavy",
        "avenir light",
        "avenir medium",
        "avenir next condensed demi bold",
        "avenir next condensed heavy",
        "avenir next condensed medium",
        "avenir next condensed ultra light",
        "avenir next condensed",
        "avenir next demi bold",
        "avenir next heavy",
        "avenir next medium",
        "avenir next ultra light",
        "avenir next",
        "avenir",
        "ayuthaya",
        "baghdad",
        "bangla mn",
        "bangla sangam mn",
        "baskerville",
        "beirut",
        "big caslon",
        "blinkmacsystemfont",
        "bodoni 72",
        "bodoni 72 oldstyle",
        "bodoni 72 smallcaps",
        "bodoni ornaments",
        "bradley hand",
        "brush script mt",
        "chalkboard se",
        "chalkboard",
        "chalkduster",
        "charter black",
        "charter",
        "cochin",
        "comic sans ms",
        "copperplate",
        "corsiva hebrew",
        "courier",
        "courier new",
        "din alternate",
        "din condensed",
        "damascus",
        "decotype naskh",
        "devanagari mt",
        "devanagari sangam mn",
        "didot",
        "diwan kufi",
        "diwan thuluth",
        "euphemia ucas",
        "farah",
        "farisi",
        "futura",
        "gb18030 bitmap",
        "galvji",
        "geeza pro",
        "geneva",
        "georgia",
        "gill sans",
        "grantha sangam mn",
        "gujarati mt",
        "gujarati sangam mn",
        "gurmukhi mn",
        "gurmukhi mt",
        "gurmukhi sangam mn",
        "heiti sc",
        "heiti tc",
        "helvetica",
        "helvetica neue",
        "herculanum",
        "hiragino kaku gothic pro w3",
        "hiragino kaku gothic pro w6",
        "hiragino kaku gothic pro",
        "hiragino kaku gothic pron w3",
        "hiragino kaku gothic pron w6",
        "hiragino kaku gothic pron",
        "hiragino kaku gothic std w8",
        "hiragino kaku gothic std",
        "hiragino kaku gothic stdn w8",
        "hiragino kaku gothic stdn",
        "hiragino maru gothic pro w4",
        "hiragino maru gothic pro",
        "hiragino maru gothic pron w4",
        "hiragino maru gothic pron",
        "hiragino mincho pro w3",
        "hiragino mincho pro w6",
        "hiragino mincho pro",
        "hiragino mincho pron w3",
        "hiragino mincho pron w6",
        "hiragino mincho pron",
        "hiragino sans gb w3",
        "hiragino sans gb w6",
        "hiragino sans gb",
        "hiragino sans w0",
        "hiragino sans w1",
        "hiragino sans w2",
        "hiragino sans w3",
        "hiragino sans w4",
        "hiragino sans w5",
        "hiragino sans w6",
        "hiragino sans w7",
        "hiragino sans w8",
        "hiragino sans w9",
        "hiragino sans",
        "hoefler text ornaments",
        "hoefler text",
        "itf devanagari marathi",
        "itf devanagari",
        "impact",
        "inaimathi",
        "iowan old style black",
        "iowan old style",
        "kailasa",
        "kannada mn",
        "kannada sangam mn",
        "kefa",
        "khmer mn",
        "khmer sangam mn",
        "kohinoor bangla",
        "kohinoor devanagari",
        "kohinoor gujarati",
        "kohinoor telugu",
        "kokonor",
        "krungthep",
        "kufistandardgk",
        "lao mn",
        "lao sangam mn",
        "lastresort",
        "lucida grande",
        "luminari",
        "malayalam mn",
        "malayalam sangam mn",
        "marion",
        "marker felt",
        "menlo",
        "microsoft sans serif",
        "mishafi gold",
        "mishafi",
        "monaco",
        "mshtakan",
        "mukta mahee",
        "muna",
        "myanmar mn",
        "myanmar sangam mn",
        "nadeem",
        "new peninim mt",
        "noteworthy",
        "noto nastaliq urdu",
        "noto sans gothic",
        "noto sans linear a",
        "noto sans linear b",
        "noto sans old italic",
        "noto serif ahom",
        "noto serif balinese",
        "noto serif myanmar",
        "optima",
        "oriya mn",
        "oriya sangam mn",
        "pt mono",
        "pt sans caption",
        "pt sans narrow",
        "pt sans",
        "pt serif caption",
        "pt serif",
        "palatino",
        "papyrus",
        "party let",
        "phosphate",
        "pingfang hk",
        "pingfang sc",
        "pingfang tc",
        "plantagenet cherokee",
        "raanana",
        "rockwell",
        "stixgeneral",
        "stixgeneral-bold",
        "stixgeneral-bolditalic",
        "stixgeneral-italic",
        "stixgeneral-regular",
        "stixintegralsd",
        "stixintegralsd-bold",
        "stixintegralsd-regular",
        "stixintegralssm",
        "stixintegralssm-bold",
        "stixintegralssm-regular",
        "stixintegralsup",
        "stixintegralsup-bold",
        "stixintegralsup-regular",
        "stixintegralsupd",
        "stixintegralsupd-bold",
        "stixintegralsupd-regular",
        "stixintegralsupsm",
        "stixintegralsupsm-bold",
        "stixintegralsupsm-regular",
        "stixnonunicode",
        "stixnonunicode-bold",
        "stixnonunicode-bolditalic",
        "stixnonunicode-italic",
        "stixnonunicode-regular",
        "stixsizefivesym",
        "stixsizefivesym-regular",
        "stixsizefoursym",
        "stixsizefoursym-bold",
        "stixsizefoursym-regular",
        "stixsizeonesym",
        "stixsizeonesym-bold",
        "stixsizeonesym-regular",
        "stixsizethreesym",
        "stixsizethreesym-bold",
        "stixsizethreesym-regular",
        "stixsizetwosym",
        "stixsizetwosym-bold",
        "stixsizetwosym-regular",
        "stixvariants",
        "stixvariants-bold",
        "stixvariants-regular",
        "stsong",
        "sana",
        "sathu",
        "savoye let plain cc.:1.0",
        "savoye let plain:1.0",
        "savoye let",
        "seravek extralight",
        "seravek light",
        "seravek medium",
        "seravek",
        "shree devanagari 714",
        "signpainter",
        "signpainter-housescript",
        "silom",
        "sinhala mn",
        "sinhala sangam mn",
        "skia",
        "snell roundhand",
        "songti sc",
        "songti tc",
        "sukhumvit set",
        "superclarendon",
        "symbol",
        "system-ui",
        "tahoma",
        "tamil mn",
        "tamil sangam mn",
        "telugu mn",
        "telugu sangam mn",
        "thonburi",
        "times new roman",
        "trattatello",
        "trebuchet ms",
        "verdana",
        "waseem",
        "webdings",
        "wingdings 2",
        "wingdings 3",
        "wingdings",
        "zapf dingbats",
        "zapfino",
    });
#elif BUILDFLAG(IS_WIN)
bool kCanRestrictFonts = true;
// This list covers the fonts installed by default on Windows 11.
// See <https://docs.microsoft.com/en-us/typography/fonts/windows_11_font_list>
base::flat_set<base::StringPiece> kFontWhitelist =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "arial",
        "arial black",
        "arial bold",
        "arial bold italic",
        "arial italic",
        "arial nova",
        "arial nova bold",
        "arial nova bold italic",
        "arial nova cond",
        "arial nova cond bold",
        "arial nova cond bold italic",
        "arial nova cond italic",
        "arial nova cond light",
        "arial nova cond light italic",
        "arial nova italic",
        "arial nova light",
        "arial nova light italic",
        "bahnschrift",
        "calibri",
        "calibri bold",
        "calibri bold italic",
        "calibri italic",
        "calibri light",
        "calibri light italic",
        "cambria",
        "cambria bold",
        "cambria bold italic",
        "cambria italic",
        "cambria math",
        "candara",
        "candara bold",
        "candara bold italic",
        "candara italic",
        "candara light",
        "candara light italic",
        "comic sans ms",
        "comic sans ms bold",
        "comic sans ms bold italic",
        "comic sans ms italic",
        "consolas",
        "consolas bold",
        "consolas bold italic",
        "consolas italic",
        "constantia",
        "constantia bold",
        "constantia bold italic",
        "constantia italic",
        "corbel",
        "corbel bold",
        "corbel bold italic",
        "corbel italic",
        "corbel light",
        "corbel light italic",
        "courier",
        "courier new",
        "courier new bold",
        "courier new bold italic",
        "courier new italic",
        "ebrima",
        "ebrima bold",
        "franklin gothic medium",
        "franklin gothic medium italic",
        "gabriola",
        "gadugi",
        "gadugi bold",
        "georgia",
        "georgia bold",
        "georgia bold italic",
        "georgia italic",
        "georgia pro",
        "georgia pro",
        "georgia pro black",
        "georgia pro black italic",
        "georgia pro bold",
        "georgia pro bold italic",
        "georgia pro cond",
        "georgia pro cond black",
        "georgia pro cond black italic",
        "georgia pro cond bold",
        "georgia pro cond bold italic",
        "georgia pro cond italic",
        "georgia pro cond light",
        "georgia pro cond light italic",
        "georgia pro cond semibold",
        "georgia pro cond semibold italic",
        "georgia pro italic",
        "georgia pro light",
        "georgia pro light italic",
        "georgia pro semibold",
        "georgia pro semibold italic",
        "gill sans nova",
        "gill sans nova",
        "gill sans nova bold",
        "gill sans nova bold italic",
        "gill sans nova cond",
        "gill sans nova cond bold",
        "gill sans nova cond bold italic",
        "gill sans nova cond italic",
        "gill sans nova cond lt",
        "gill sans nova cond lt italic",
        "gill sans nova cond ultra bold",
        "gill sans nova cond xbd",
        "gill sans nova cond xbd italic",
        "gill sans nova italic",
        "gill sans nova light",
        "gill sans nova light italic",
        "gill sans nova ultra bold",
        "helvetica",
        "hololens mdl2 assets",
        "impact",
        "ink free",
        "javanese text",
        "leelawadee ui",
        "leelawadee ui bold",
        "leelawadee ui semilight",
        "lucida console",
        "lucida sans unicode",
        "ms gothic",
        "ms pgothic",
        "ms ui gothic",
        "mv boli",
        "malgun gothic",
        "malgun gothic bold",
        "malgun gothic semilight",
        "marlett",
        "microsoft himalaya",
        "microsoft jhenghei",
        "microsoft jhenghei bold",
        "microsoft jhenghei light",
        "microsoft jhenghei ui",
        "microsoft jhenghei ui bold",
        "microsoft jhenghei ui light",
        "microsoft new tai lue",
        "microsoft new tai lue bold",
        "microsoft phagspa",
        "microsoft phagspa bold",
        "microsoft sans serif",
        "microsoft tai le",
        "microsoft tai le bold",
        "microsoft yahei",
        "microsoft yahei bold",
        "microsoft yahei light",
        "microsoft yahei ui",
        "microsoft yahei ui bold",
        "microsoft yahei ui light",
        "microsoft yi baiti",
        "mingliu-extb",
        "mingliu_hkscs-extb",
        "mongolian baiti",
        "myanmar text",
        "myanmar text bold",
        "nsimsun",
        "neue haas grotesk text pro",
        "neue haas grotesk text pro black",
        "neue haas grotesk text pro black italic",
        "neue haas grotesk text pro bold",
        "neue haas grotesk text pro bold italic",
        "neue haas grotesk text pro extralight",
        "neue haas grotesk text pro extralight italic",
        "neue haas grotesk text pro light",
        "neue haas grotesk text pro light italic",
        "neue haas grotesk text pro medium",
        "neue haas grotesk text pro medium italic",
        "neue haas grotesk text pro regular",
        "neue haas grotesk text pro regular italic",
        "neue haas grotesk text pro thin",
        "neue haas grotesk text pro thin italic",
        "neue haas grotesk text pro ultrathin",
        "neue haas grotesk text pro ultrathin italic",
        "nirmala ui",
        "nirmala ui bold",
        "nirmala ui semilight",
        "pmingliu-extb",
        "palatino linotype",
        "palatino linotype bold",
        "palatino linotype bold italic",
        "palatino linotype italic",
        "rockwell nova",
        "rockwell nova bold",
        "rockwell nova bold italic",
        "rockwell nova cond",
        "rockwell nova cond bold",
        "rockwell nova cond bold italic",
        "rockwell nova cond italic",
        "rockwell nova cond light",
        "rockwell nova cond light italic",
        "rockwell nova extra bold",
        "rockwell nova extra bold italic",
        "rockwell nova italic",
        "rockwell nova light",
        "rockwell nova light italic",
        "segoe fluent icons",
        "segoe mdl2 assets",
        "segoe print",
        "segoe print bold",
        "segoe script",
        "segoe script bold",
        "segoe ui",
        "segoe ui",
        "segoe ui black",
        "segoe ui black italic",
        "segoe ui bold",
        "segoe ui bold italic",
        "segoe ui emoji",
        "segoe ui historic",
        "segoe ui italic",
        "segoe ui light",
        "segoe ui light italic",
        "segoe ui semibold",
        "segoe ui semibold italic",
        "segoe ui semilight",
        "segoe ui semilight italic",
        "segoe ui symbol",
        "segoe ui variable",
        "segoe ui variable display bold",
        "segoe ui variable display light",
        "segoe ui variable display regular",
        "segoe ui variable display semibold",
        "segoe ui variable display semilight",
        "segoe ui variable small bold",
        "segoe ui variable small light",
        "segoe ui variable small regular",
        "segoe ui variable small semibold",
        "segoe ui variable small semilight",
        "segoe ui variable text bold",
        "segoe ui variable text light",
        "segoe ui variable text regular",
        "segoe ui variable text semibold",
        "segoe ui variable text semilight",
        "simsun",
        "simsun-extb",
        "sitka",
        "sitka banner",
        "sitka banner bold",
        "sitka banner bold italic",
        "sitka banner italic",
        "sitka banner semibold",
        "sitka banner semibold italic",
        "sitka display",
        "sitka display bold",
        "sitka display bold italic",
        "sitka display italic",
        "sitka display semibold",
        "sitka display semibold italic",
        "sitka heading",
        "sitka heading bold",
        "sitka heading bold italic",
        "sitka heading italic",
        "sitka heading semibold",
        "sitka heading semibold italic",
        "sitka small",
        "sitka small bold",
        "sitka small bold italic",
        "sitka small italic",
        "sitka small semibold",
        "sitka small semibold italic",
        "sitka subheading",
        "sitka subheading bold",
        "sitka subheading bold italic",
        "sitka subheading italic",
        "sitka subheading semibold",
        "sitka subheading semibold italic",
        "sitka text",
        "sitka text bold",
        "sitka text bold italic",
        "sitka text italic",
        "sitka text semibold",
        "sitka text semibold italic",
        "sylfaen",
        "symbol",
        "tahoma",
        "tahoma bold",
        "times new roman",
        "times new roman bold",
        "times new roman bold italic",
        "times new roman italic",
        "trebuchet ms",
        "trebuchet ms bold",
        "trebuchet ms bold italic",
        "trebuchet ms italic",
        "verdana",
        "verdana bold",
        "verdana bold italic",
        "verdana italic",
        "verdana pro",
        "verdana pro",
        "verdana pro black",
        "verdana pro black italic",
        "verdana pro bold",
        "verdana pro bold italic",
        "verdana pro cond",
        "verdana pro cond black",
        "verdana pro cond black italic",
        "verdana pro cond bold",
        "verdana pro cond bold italic",
        "verdana pro cond italic",
        "verdana pro cond light",
        "verdana pro cond light italic",
        "verdana pro cond semibold",
        "verdana pro cond semibold italic",
        "verdana pro italic",
        "verdana pro light",
        "verdana pro light italic",
        "verdana pro semibold",
        "verdana pro semibold italic",
        "webdings",
        "wingdings",
        "yu gothic",
        "yu gothic bold",
        "yu gothic light",
        "yu gothic medium",
        "yu gothic regular",
        "yu gothic ui bold",
        "yu gothic ui light",
        "yu gothic ui regular",
        "yu gothic ui semibold",
        "yu gothic ui semilight"});
#elif BUILDFLAG(IS_ANDROID)
bool kCanRestrictFonts = true;
// This list covers the fonts and font aliases listed in data/fonts/fonts.xml of
// the Android Open Source Project. To reduce memory and maintenance, most
// region-specific Noto fonts are handled by wildcards outside this list.
base::flat_set<base::StringPiece> kFontWhitelist =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"androidclock",
                                       "arial",
                                       "baskerville",
                                       "carrois gothic",
                                       "coming soon",
                                       "courier",
                                       "courier new",
                                       "cutive mono",
                                       "dancing script",
                                       "droid sans",
                                       "droid sans mono",
                                       "erif-bold",
                                       "fantasy",
                                       "georgia",
                                       "goudy",
                                       "helvetica",
                                       "itc stone serif",
                                       "monaco",
                                       "noto color emoji",
                                       "noto kufi arabic",
                                       "noto naskh arabic",
                                       "noto nastaliq urdu",
                                       "noto sans",
                                       "noto serif",
                                       "palatino",
                                       "roboto",
                                       "roboto static",
                                       "sans-serif-black",
                                       "sans-serif-condensed-light",
                                       "sans-serif-condensed-medium",
                                       "sans-serif-light",
                                       "sans-serif-medium",
                                       "sans-serif-monospace",
                                       "sans-serif-thin",
                                       "source sans pro",
                                       "source-sans-pro-semi-bold",
                                       "tahoma",
                                       "times",
                                       "times new roman",
                                       "verdana"});
#else
bool kCanRestrictFonts = false;
base::flat_set<base::StringPiece> kFontWhitelist =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
#endif

#if BUILDFLAG(IS_WIN)
base::flat_set<base::StringPiece> kFontWhitelistAR =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "aldhabi", "andalus", "arabic typesetting", "microsoft uighur",
        "microsoft uighur bold", "sakkal majalla", "sakkal majalla bold",
        "simplified arabic", "simplified arabic bold",
        "simplified arabic fixed", "traditional arabic",
        "traditional arabic bold", "urdu typesetting",
        "urdu typesetting bold"});
base::flat_set<base::StringPiece> kFontWhitelistAS =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "shonar bangla", "shonar bangla bold", "vrinda", "vrinda bold"});
base::flat_set<base::StringPiece> kFontWhitelistIU =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"euphemia"});
base::flat_set<base::StringPiece> kFontWhitelistHI =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "aparajita", "aparajita italic", "aparajita bold",
        "aparajita bold italic", "kokila", "kokila italic", "kokila bold",
        "kokila bold italic", "mangal", "mangal bold", "sanskrit text",
        "utsaah", "utsaah italic", "utsaah bold", "utsaah bold italic"});
base::flat_set<base::StringPiece> kFontWhitelistAM =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"nyala"});
base::flat_set<base::StringPiece> kFontWhitelistGU =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"shruti", "shruti bold"});
base::flat_set<base::StringPiece> kFontWhitelistPA =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"raavi", "raavi bold"});
base::flat_set<base::StringPiece> kFontWhitelistZH =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "dengxian light", "dengxian", "dengxian bold", "fangsong", "kaiti",
        "simhei", "dfkai-sb", "mingliu", "mingliu_hkscs", "pmingliu"});
base::flat_set<base::StringPiece> kFontWhitelistHE =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "aharoni bold", "david", "david bold", "frankruehl", "gisha",
        "gisha bold", "levenim mt", "levenim mt bold", "miriam", "miriam fixed",
        "narkisim", "rod"});
base::flat_set<base::StringPiece> kFontWhitelistJA =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"biz udgothic",
                                       "biz udgothic bold",
                                       "biz udpgothic",
                                       "biz udpgothic bold",
                                       "biz udmincho medium",
                                       "biz udpmincho medium",
                                       "meiryo",
                                       "meiryo italic",
                                       "meiryo bold",
                                       "meiryo bold italic",
                                       "meiryo ui",
                                       "meiryo ui italic",
                                       "meiryo ui bold",
                                       "meiryo ui bold italic",
                                       "ms mincho",
                                       "ms pmincho",
                                       "ud digi kyokasho",
                                       "ud digi kyokasho n-b",
                                       "ud digi kyokasho nk-b",
                                       "ud digi kyokasho nk-r",
                                       "ud digi kyokasho np-b",
                                       "ud digi kyokasho np-r",
                                       "ud digi kyokasho n-r",
                                       "yu mincho light",
                                       "yu mincho regular",
                                       "yu mincho demibold"});
base::flat_set<base::StringPiece> kFontWhitelistKN =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"tunga", "tunga bold"});
base::flat_set<base::StringPiece> kFontWhitelistKM =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "daunpenh", "khmer ui", "khmer ui bold", "moolboran"});
base::flat_set<base::StringPiece> kFontWhitelistKO =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "batang", "batangche", "dotum", "dotumche", "gulim", "gulimche",
        "gungsuh", "gungsuhche"});
base::flat_set<base::StringPiece> kFontWhitelistLO =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"dokchampa", "lao ui", "lao ui bold"});
base::flat_set<base::StringPiece> kFontWhitelistML =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"kartika", "kartika bold"});
#else
base::flat_set<base::StringPiece> kFontWhitelistAR =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistAS =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistIU =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistHI =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistAM =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistGU =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistPA =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistZH =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistHE =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistJA =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistKN =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistKM =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistKO =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistLO =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kFontWhitelistML =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
#endif
}  // namespace

bool AllowFontByFamilyName(const AtomicString& family_name,
                           WTF::String default_language) {
  if (!kCanRestrictFonts)
    return true;
  std::string lower_ascii_name = family_name.LowerASCII().Ascii();
  if (kFontWhitelist.contains(lower_ascii_name))
    return true;
  if (GetAdditionalFontWhitelistByLocale(default_language)
          .contains(lower_ascii_name))
    return true;
#if BUILDFLAG(IS_ANDROID)
  // There are literally hundreds of region-specific Noto fonts.
  // To reduce memory and maintenance, we allow them by wildcard.
  if (family_name.StartsWithIgnoringASCIICase("noto sans ") ||
      family_name.StartsWithIgnoringASCIICase("noto serif "))
    return true;
#endif
  return false;
}

const base::flat_set<base::StringPiece>& GetAdditionalFontWhitelistByLocale(
    WTF::String locale_language) {
  if (locale_language == "ar" || locale_language == "fa" ||
      locale_language == "ur")
    return kFontWhitelistAR;
  if (locale_language == "as")
    return kFontWhitelistAS;
  if (locale_language == "iu")
    return kFontWhitelistIU;
  if (locale_language == "hi" || locale_language == "mr")
    return kFontWhitelistHI;
  if (locale_language == "am" || locale_language == "ti")
    return kFontWhitelistAM;
  if (locale_language == "gu")
    return kFontWhitelistGU;
  if (locale_language == "pa")
    return kFontWhitelistPA;
  if (locale_language == "zh")
    return kFontWhitelistZH;
  if (locale_language == "he")
    return kFontWhitelistHE;
  if (locale_language == "ja")
    return kFontWhitelistJA;
  if (locale_language == "kn")
    return kFontWhitelistKN;
  if (locale_language == "km")
    return kFontWhitelistKM;
  if (locale_language == "ko")
    return kFontWhitelistKO;
  if (locale_language == "lo")
    return kFontWhitelistLO;
  if (locale_language == "ml")
    return kFontWhitelistML;
  return kEmptyFontSet;
}

void set_font_whitelist_for_testing(
    bool can_restrict_fonts,
    const base::flat_set<base::StringPiece>& font_whitelist) {
  kCanRestrictFonts = can_restrict_fonts;
  kFontWhitelist = font_whitelist;
}

bool get_can_restrict_fonts_for_testing() {
  return kCanRestrictFonts;
}

const base::flat_set<base::StringPiece>& get_font_whitelist_for_testing() {
  return kFontWhitelist;
}

}  // namespace brave
