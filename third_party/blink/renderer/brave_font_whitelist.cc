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
base::flat_set<base::StringPiece> kAllowedFontFamilies =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Academy Engraved LET",
        "Al Bayan",
        "Al Nile",
        "Al Tarikh",
        "American Typewriter",
        "Andale Mono",
        "Apple Braille Outline 6 Dot",
        "Apple Braille Outline 8 Dot",
        "Apple Braille Pinpoint 6 Dot",
        "Apple Braille Pinpoint 8 Dot",
        "Apple Braille",
        "Apple Chancery",
        "Apple Color Emoji",
        "Apple SD Gothic Neo",
        "Apple Symbols",
        "AppleGothic",
        "AppleMyungjo",
        "AquaKana",
        "Arial Black",
        "Arial Hebrew Scholar",
        "Arial Hebrew",
        "Arial Narrow",
        "Arial Rounded MT Bold",
        "Arial Unicode MS",
        "Arial",
        "Athelas",
        "Avenir Black Oblique",
        "Avenir Black",
        "Avenir Book",
        "Avenir Heavy",
        "Avenir Light",
        "Avenir Medium",
        "Avenir Next Condensed Demi Bold",
        "Avenir Next Condensed Heavy",
        "Avenir Next Condensed Medium",
        "Avenir Next Condensed Ultra Light",
        "Avenir Next Condensed",
        "Avenir Next Demi Bold",
        "Avenir Next Heavy",
        "Avenir Next Medium",
        "Avenir Next Ultra Light",
        "Avenir Next",
        "Avenir",
        "Ayuthaya",
        "Baghdad",
        "Bangla MN",
        "Bangla Sangam MN",
        "Baskerville",
        "Beirut",
        "Big Caslon",
        "Bodoni 72",
        "Bodoni 72 Oldstyle",
        "Bodoni 72 Smallcaps",
        "Bodoni Ornaments",
        "Bradley Hand",
        "Brush Script MT",
        "Chalkboard SE",
        "Chalkboard",
        "Chalkduster",
        "Charter Black",
        "Charter",
        "Cochin",
        "Comic Sans MS",
        "Copperplate",
        "Corsiva Hebrew",
        "Courier New",
        "DIN Alternate",
        "DIN Condensed",
        "Damascus",
        "DecoType Naskh",
        "Devanagari MT",
        "Devanagari Sangam MN",
        "Didot",
        "Diwan Kufi",
        "Diwan Thuluth",
        "Euphemia UCAS",
        "Farah",
        "Farisi",
        "Futura",
        "GB18030 Bitmap",
        "Galvji",
        "Geeza Pro",
        "Geneva",
        "Georgia",
        "Gill Sans",
        "Grantha Sangam MN",
        "Gujarati MT",
        "Gujarati Sangam MN",
        "Gurmukhi MN",
        "Gurmukhi MT",
        "Gurmukhi Sangam MN",
        "Heiti SC",
        "Heiti TC",
        "Helvetica",
        "Helvetica Neue",
        "Herculanum",
        "Hiragino Kaku Gothic Pro W3",
        "Hiragino Kaku Gothic Pro W6",
        "Hiragino Kaku Gothic Pro",
        "Hiragino Kaku Gothic ProN W3",
        "Hiragino Kaku Gothic ProN W6",
        "Hiragino Kaku Gothic ProN",
        "Hiragino Kaku Gothic Std W8",
        "Hiragino Kaku Gothic Std",
        "Hiragino Kaku Gothic StdN W8",
        "Hiragino Kaku Gothic StdN",
        "Hiragino Maru Gothic Pro W4",
        "Hiragino Maru Gothic Pro",
        "Hiragino Maru Gothic ProN W4",
        "Hiragino Maru Gothic ProN",
        "Hiragino Mincho Pro W3",
        "Hiragino Mincho Pro W6",
        "Hiragino Mincho Pro",
        "Hiragino Mincho ProN W3",
        "Hiragino Mincho ProN W6",
        "Hiragino Mincho ProN",
        "Hiragino Sans GB W3",
        "Hiragino Sans GB W6",
        "Hiragino Sans GB",
        "Hiragino Sans W0",
        "Hiragino Sans W1",
        "Hiragino Sans W2",
        "Hiragino Sans W3",
        "Hiragino Sans W4",
        "Hiragino Sans W5",
        "Hiragino Sans W6",
        "Hiragino Sans W7",
        "Hiragino Sans W8",
        "Hiragino Sans W9",
        "Hiragino Sans",
        "Hoefler Text Ornaments",
        "Hoefler Text",
        "ITF Devanagari Marathi",
        "ITF Devanagari",
        "Impact",
        "InaiMathi",
        "Iowan Old Style Black",
        "Iowan Old Style",
        "Kailasa",
        "Kannada MN",
        "Kannada Sangam MN",
        "Kefa",
        "Khmer MN",
        "Khmer Sangam MN",
        "Kohinoor Bangla",
        "Kohinoor Devanagari",
        "Kohinoor Gujarati",
        "Kohinoor Telugu",
        "Kokonor",
        "Krungthep",
        "KufiStandardGK",
        "Lao MN",
        "Lao Sangam MN",
        "LastResort",
        "Lucida Grande",
        "Luminari",
        "Malayalam MN",
        "Malayalam Sangam MN",
        "Marion",
        "Marker Felt",
        "Menlo",
        "Microsoft Sans Serif",
        "Mishafi Gold",
        "Mishafi",
        "Monaco",
        "Mshtakan",
        "Mukta Mahee",
        "Muna",
        "Myanmar MN",
        "Myanmar Sangam MN",
        "Nadeem",
        "New Peninim MT",
        "Noteworthy",
        "Noto Nastaliq Urdu",
        "Noto Sans Gothic",
        "Noto Sans Linear A",
        "Noto Sans Linear B",
        "Noto Sans Old Italic",
        "Noto Serif Ahom",
        "Noto Serif Balinese",
        "Noto Serif Myanmar",
        "Optima",
        "Oriya MN",
        "Oriya Sangam MN",
        "PT Mono",
        "PT Sans Caption",
        "PT Sans Narrow",
        "PT Sans",
        "PT Serif Caption",
        "PT Serif",
        "Palatino",
        "Papyrus",
        "Party LET",
        "Phosphate",
        "PingFang HK",
        "PingFang SC",
        "PingFang TC",
        "Plantagenet Cherokee",
        "Raanana",
        "Rockwell",
        "STIXGeneral",
        "STIXGeneral-Bold",
        "STIXGeneral-BoldItalic",
        "STIXGeneral-Italic",
        "STIXGeneral-Regular",
        "STIXIntegralsD",
        "STIXIntegralsD-Bold",
        "STIXIntegralsD-Regular",
        "STIXIntegralsSm",
        "STIXIntegralsSm-Bold",
        "STIXIntegralsSm-Regular",
        "STIXIntegralsUp",
        "STIXIntegralsUp-Bold",
        "STIXIntegralsUp-Regular",
        "STIXIntegralsUpD",
        "STIXIntegralsUpD-Bold",
        "STIXIntegralsUpD-Regular",
        "STIXIntegralsUpSm",
        "STIXIntegralsUpSm-Bold",
        "STIXIntegralsUpSm-Regular",
        "STIXNonUnicode",
        "STIXNonUnicode-Bold",
        "STIXNonUnicode-BoldItalic",
        "STIXNonUnicode-Italic",
        "STIXNonUnicode-Regular",
        "STIXSizeFiveSym",
        "STIXSizeFiveSym-Regular",
        "STIXSizeFourSym",
        "STIXSizeFourSym-Bold",
        "STIXSizeFourSym-Regular",
        "STIXSizeOneSym",
        "STIXSizeOneSym-Bold",
        "STIXSizeOneSym-Regular",
        "STIXSizeThreeSym",
        "STIXSizeThreeSym-Bold",
        "STIXSizeThreeSym-Regular",
        "STIXSizeTwoSym",
        "STIXSizeTwoSym-Bold",
        "STIXSizeTwoSym-Regular",
        "STIXVariants",
        "STIXVariants-Bold",
        "STIXVariants-Regular",
        "STSong",
        "Sana",
        "Sathu",
        "Savoye LET Plain CC.:1.0",
        "Savoye LET Plain:1.0",
        "Savoye LET",
        "Seravek ExtraLight",
        "Seravek Light",
        "Seravek Medium",
        "Seravek",
        "Shree Devanagari 714",
        "SignPainter",
        "SignPainter-HouseScript",
        "Silom",
        "Sinhala MN",
        "Sinhala Sangam MN",
        "Skia",
        "Snell Roundhand",
        "Songti SC",
        "Songti TC",
        "Sukhumvit Set",
        "Superclarendon",
        "Symbol",
        "Tahoma",
        "Tamil MN",
        "Tamil Sangam MN",
        "Telugu MN",
        "Telugu Sangam MN",
        "Thonburi",
        "Times New Roman",
        "Trattatello",
        "Trebuchet MS",
        "Verdana",
        "Waseem",
        "Webdings",
        "Wingdings 2",
        "Wingdings 3",
        "Wingdings",
        "Zapf Dingbats",
        "Zapfino",
    });
#elif BUILDFLAG(IS_WIN)
bool kCanRestrictFonts = true;
// This list covers the fonts installed by default on Windows 11.
// See <https://docs.microsoft.com/en-us/typography/fonts/windows_11_font_list>
base::flat_set<base::StringPiece> kAllowedFontFamilies =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Arial",
        "Arial Black",
        "Arial Bold",
        "Arial Bold Italic",
        "Arial Italic",
        "Arial Nova",
        "Arial Nova Bold",
        "Arial Nova Bold Italic",
        "Arial Nova Cond",
        "Arial Nova Cond Bold",
        "Arial Nova Cond Bold Italic",
        "Arial Nova Cond Italic",
        "Arial Nova Cond Light",
        "Arial Nova Cond Light Italic",
        "Arial Nova Italic",
        "Arial Nova Light",
        "Arial Nova Light Italic",
        "Bahnschrift",
        "Calibri",
        "Calibri Bold",
        "Calibri Bold Italic",
        "Calibri Italic",
        "Calibri Light",
        "Calibri Light Italic",
        "Cambria",
        "Cambria Bold",
        "Cambria Bold Italic",
        "Cambria Italic",
        "Cambria Math",
        "Candara",
        "Candara Bold",
        "Candara Bold Italic",
        "Candara Italic",
        "Candara Light",
        "Candara Light Italic",
        "Comic Sans MS",
        "Comic Sans MS Bold",
        "Comic Sans MS Bold Italic",
        "Comic Sans MS Italic",
        "Consolas",
        "Consolas Bold",
        "Consolas Bold Italic",
        "Consolas Italic",
        "Constantia",
        "Constantia Bold",
        "Constantia Bold Italic",
        "Constantia Italic",
        "Corbel",
        "Corbel Bold",
        "Corbel Bold Italic",
        "Corbel Italic",
        "Corbel Light",
        "Corbel Light Italic",
        "Courier New",
        "Courier New Bold",
        "Courier New Bold Italic",
        "Courier New Italic",
        "Ebrima",
        "Ebrima Bold",
        "Franklin Gothic Medium",
        "Franklin Gothic Medium Italic",
        "Gabriola",
        "Gadugi",
        "Gadugi Bold",
        "Georgia",
        "Georgia Bold",
        "Georgia Bold Italic",
        "Georgia Italic",
        "Georgia Pro",
        "Georgia Pro",
        "Georgia Pro Black",
        "Georgia Pro Black Italic",
        "Georgia Pro Bold",
        "Georgia Pro Bold Italic",
        "Georgia Pro Cond",
        "Georgia Pro Cond Black",
        "Georgia Pro Cond Black Italic",
        "Georgia Pro Cond Bold",
        "Georgia Pro Cond Bold Italic",
        "Georgia Pro Cond Italic",
        "Georgia Pro Cond Light",
        "Georgia Pro Cond Light Italic",
        "Georgia Pro Cond Semibold",
        "Georgia Pro Cond Semibold Italic",
        "Georgia Pro Italic",
        "Georgia Pro Light",
        "Georgia Pro Light Italic",
        "Georgia Pro Semibold",
        "Georgia Pro Semibold Italic",
        "Gill Sans Nova",
        "Gill Sans Nova",
        "Gill Sans Nova Bold",
        "Gill Sans Nova Bold Italic",
        "Gill Sans Nova Cond",
        "Gill Sans Nova Cond Bold",
        "Gill Sans Nova Cond Bold Italic",
        "Gill Sans Nova Cond Italic",
        "Gill Sans Nova Cond Lt",
        "Gill Sans Nova Cond Lt Italic",
        "Gill Sans Nova Cond Ultra Bold",
        "Gill Sans Nova Cond XBd",
        "Gill Sans Nova Cond XBd Italic",
        "Gill Sans Nova Italic",
        "Gill Sans Nova Light",
        "Gill Sans Nova Light Italic",
        "Gill Sans Nova Ultra Bold",
        "Helvetica",
        "HoloLens MDL2 Assets",
        "Impact",
        "Ink Free",
        "Javanese Text",
        "Leelawadee UI",
        "Leelawadee UI Bold",
        "Leelawadee UI Semilight",
        "Lucida Console",
        "Lucida Sans Unicode",
        "MS Gothic",
        "MS PGothic",
        "MS UI Gothic",
        "MV Boli",
        "Malgun Gothic",
        "Malgun Gothic Bold",
        "Malgun Gothic Semilight",
        "Marlett",
        "Microsoft Himalaya",
        "Microsoft JhengHei",
        "Microsoft JhengHei Bold",
        "Microsoft JhengHei Light",
        "Microsoft JhengHei UI",
        "Microsoft JhengHei UI Bold",
        "Microsoft JhengHei UI Light",
        "Microsoft New Tai Lue",
        "Microsoft New Tai Lue Bold",
        "Microsoft PhagsPa",
        "Microsoft PhagsPa Bold",
        "Microsoft Sans Serif",
        "Microsoft Tai Le",
        "Microsoft Tai Le Bold",
        "Microsoft YaHei",
        "Microsoft YaHei Bold",
        "Microsoft YaHei Light",
        "Microsoft YaHei UI",
        "Microsoft YaHei UI Bold",
        "Microsoft YaHei UI Light",
        "Microsoft Yi Baiti",
        "MingLiU-ExtB",
        "MingLiU_HKSCS-ExtB",
        "Mongolian Baiti",
        "Myanmar Text",
        "Myanmar Text Bold",
        "NSimSun",
        "Neue Haas Grotesk Text Pro",
        "Neue Haas Grotesk Text Pro Black",
        "Neue Haas Grotesk Text Pro Black Italic",
        "Neue Haas Grotesk Text Pro Bold",
        "Neue Haas Grotesk Text Pro Bold Italic",
        "Neue Haas Grotesk Text Pro ExtraLight",
        "Neue Haas Grotesk Text Pro ExtraLight Italic",
        "Neue Haas Grotesk Text Pro Light",
        "Neue Haas Grotesk Text Pro Light Italic",
        "Neue Haas Grotesk Text Pro Medium",
        "Neue Haas Grotesk Text Pro Medium Italic",
        "Neue Haas Grotesk Text Pro Regular",
        "Neue Haas Grotesk Text Pro Regular Italic",
        "Neue Haas Grotesk Text Pro Thin",
        "Neue Haas Grotesk Text Pro Thin Italic",
        "Neue Haas Grotesk Text Pro UltraThin",
        "Neue Haas Grotesk Text Pro UltraThin Italic",
        "Nirmala UI",
        "Nirmala UI Bold",
        "Nirmala UI Semilight",
        "PMingLiU-ExtB",
        "Palatino Linotype",
        "Palatino Linotype Bold",
        "Palatino Linotype Bold Italic",
        "Palatino Linotype Italic",
        "Rockwell Nova",
        "Rockwell Nova Bold",
        "Rockwell Nova Bold Italic",
        "Rockwell Nova Cond",
        "Rockwell Nova Cond Bold",
        "Rockwell Nova Cond Bold Italic",
        "Rockwell Nova Cond Italic",
        "Rockwell Nova Cond Light",
        "Rockwell Nova Cond Light Italic",
        "Rockwell Nova Extra Bold",
        "Rockwell Nova Extra Bold Italic",
        "Rockwell Nova Italic",
        "Rockwell Nova Light",
        "Rockwell Nova Light Italic",
        "Segoe Fluent Icons",
        "Segoe MDL2 Assets",
        "Segoe Print",
        "Segoe Print Bold",
        "Segoe Script",
        "Segoe Script Bold",
        "Segoe UI",
        "Segoe UI",
        "Segoe UI Black",
        "Segoe UI Black Italic",
        "Segoe UI Bold",
        "Segoe UI Bold Italic",
        "Segoe UI Emoji",
        "Segoe UI Historic",
        "Segoe UI Italic",
        "Segoe UI Light",
        "Segoe UI Light Italic",
        "Segoe UI Semibold",
        "Segoe UI Semibold Italic",
        "Segoe UI Semilight",
        "Segoe UI Semilight Italic",
        "Segoe UI Symbol",
        "Segoe UI Variable",
        "Segoe UI Variable Display Bold",
        "Segoe UI Variable Display Light",
        "Segoe UI Variable Display Regular",
        "Segoe UI Variable Display Semibold",
        "Segoe UI Variable Display Semilight",
        "Segoe UI Variable Small Bold",
        "Segoe UI Variable Small Light",
        "Segoe UI Variable Small Regular",
        "Segoe UI Variable Small Semibold",
        "Segoe UI Variable Small Semilight",
        "Segoe UI Variable Text Bold",
        "Segoe UI Variable Text Light",
        "Segoe UI Variable Text Regular",
        "Segoe UI Variable Text Semibold",
        "Segoe UI Variable Text Semilight",
        "SimSun",
        "SimSun-ExtB",
        "Sitka",
        "Sitka Banner",
        "Sitka Banner Bold",
        "Sitka Banner Bold Italic",
        "Sitka Banner Italic",
        "Sitka Banner Semibold",
        "Sitka Banner Semibold Italic",
        "Sitka Display",
        "Sitka Display Bold",
        "Sitka Display Bold Italic",
        "Sitka Display Italic",
        "Sitka Display Semibold",
        "Sitka Display Semibold Italic",
        "Sitka Heading",
        "Sitka Heading Bold",
        "Sitka Heading Bold Italic",
        "Sitka Heading Italic",
        "Sitka Heading Semibold",
        "Sitka Heading Semibold Italic",
        "Sitka Small",
        "Sitka Small Bold",
        "Sitka Small Bold Italic",
        "Sitka Small Italic",
        "Sitka Small Semibold",
        "Sitka Small Semibold Italic",
        "Sitka Subheading",
        "Sitka Subheading Bold",
        "Sitka Subheading Bold Italic",
        "Sitka Subheading Italic",
        "Sitka Subheading Semibold",
        "Sitka Subheading Semibold Italic",
        "Sitka Text",
        "Sitka Text Bold",
        "Sitka Text Bold Italic",
        "Sitka Text Italic",
        "Sitka Text Semibold",
        "Sitka Text Semibold Italic",
        "Sylfaen",
        "Symbol",
        "Tahoma",
        "Tahoma Bold",
        "Times New Roman",
        "Times New Roman Bold",
        "Times New Roman Bold Italic",
        "Times New Roman Italic",
        "Trebuchet MS",
        "Trebuchet MS Bold",
        "Trebuchet MS Bold Italic",
        "Trebuchet MS Italic",
        "Verdana",
        "Verdana Bold",
        "Verdana Bold Italic",
        "Verdana Italic",
        "Verdana Pro",
        "Verdana Pro",
        "Verdana Pro Black",
        "Verdana Pro Black Italic",
        "Verdana Pro Bold",
        "Verdana Pro Bold Italic",
        "Verdana Pro Cond",
        "Verdana Pro Cond Black",
        "Verdana Pro Cond Black Italic",
        "Verdana Pro Cond Bold",
        "Verdana Pro Cond Bold Italic",
        "Verdana Pro Cond Italic",
        "Verdana Pro Cond Light",
        "Verdana Pro Cond Light Italic",
        "Verdana Pro Cond SemiBold",
        "Verdana Pro Cond SemiBold Italic",
        "Verdana Pro Italic",
        "Verdana Pro Light",
        "Verdana Pro Light Italic",
        "Verdana Pro SemiBold",
        "Verdana Pro SemiBold Italic",
        "Webdings",
        "Wingdings",
        "Yu Gothic",
        "Yu Gothic Bold",
        "Yu Gothic Light",
        "Yu Gothic Medium",
        "Yu Gothic Regular",
        "Yu Gothic UI Bold",
        "Yu Gothic UI Light",
        "Yu Gothic UI Regular",
        "Yu Gothic UI Semibold",
        "Yu Gothic UI Semilight"});
#else
bool kCanRestrictFonts = false;
base::flat_set<base::StringPiece> kAllowedFontFamilies =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
#endif

#if BUILDFLAG(IS_WIN)
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesAR =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Aldhabi", "Andalus", "Arabic Typesetting", "Microsoft Uighur",
        "Microsoft Uighur Bold", "Sakkal Majalla", "Sakkal Majalla Bold",
        "Simplified Arabic", "Simplified Arabic Bold",
        "Simplified Arabic Fixed", "Traditional Arabic",
        "Traditional Arabic Bold", "Urdu Typesetting",
        "Urdu Typesetting Bold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesAS =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Shonar Bangla", "Shonar Bangla Bold", "Vrinda", "Vrinda Bold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesIU =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"Euphemia"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesHI =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Aparajita", "Aparajita Italic", "Aparajita Bold",
        "Aparajita Bold Italic", "Kokila", "Kokila Italic", "Kokila Bold",
        "Kokila Bold Italic", "Mangal", "Mangal Bold", "Sanskrit Text",
        "Utsaah", "Utsaah Italic", "Utsaah Bold", "Utsaah Bold Italic"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesAM =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"Nyala"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesGU =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"Shruti", "Shruti Bold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesPA =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"Raavi", "Raavi Bold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesZH =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "DengXian Light", "DengXian", "DengXian Bold", "FangSong", "KaiTi",
        "SimHei", "DFKai-SB", "MingLiU", "MingLiU_HKSCS", "PMingLiU"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesHE =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Aharoni Bold", "David", "David Bold", "FrankRuehl", "Gisha",
        "Gisha Bold", "Levenim MT", "Levenim MT Bold", "Miriam", "Miriam Fixed",
        "Narkisim", "Rod"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesJA =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"BIZ UDGothic",
                                       "BIZ UDGothic Bold",
                                       "BIZ UDPGothic",
                                       "BIZ UDPGothic Bold",
                                       "BIZ UDMincho Medium",
                                       "BIZ UDPMincho Medium",
                                       "Meiryo",
                                       "Meiryo Italic",
                                       "Meiryo Bold",
                                       "Meiryo Bold Italic",
                                       "Meiryo UI",
                                       "Meiryo UI Italic",
                                       "Meiryo UI Bold",
                                       "Meiryo UI Bold Italic",
                                       "MS Mincho",
                                       "MS PMincho",
                                       "UD Digi Kyokasho",
                                       "UD Digi Kyokasho N-B",
                                       "UD Digi Kyokasho NK-B",
                                       "UD Digi Kyokasho NK-R",
                                       "UD Digi Kyokasho NP-B",
                                       "UD Digi Kyokasho NP-R",
                                       "UD Digi Kyokasho N-R",
                                       "Yu Mincho Light",
                                       "Yu Mincho Regular",
                                       "Yu Mincho Demibold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesKN =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"Tunga", "Tunga Bold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesKM =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "DaunPenh", "Khmer UI", "Khmer UI Bold", "MoolBoran"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesKO =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{
        "Batang", "BatangChe", "Dotum", "DotumChe", "Gulim", "GulimChe",
        "Gungsuh", "GungsuhChe"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesLO =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"DokChampa", "Lao UI", "Lao UI Bold"});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesML =
    base::MakeFlatSet<base::StringPiece>(
        std::vector<base::StringPiece>{"Kartika", "Kartika Bold"});
#else
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesAR =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesAS =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesIU =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesHI =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesAM =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesGU =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesPA =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesZH =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesHE =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesJA =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesKN =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesKM =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesKO =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesLO =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
base::flat_set<base::StringPiece> kAdditionalAllowedFontFamiliesML =
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{});
#endif
}  // namespace

bool CanRestrictFontFamiliesOnThisPlatform() {
  return kCanRestrictFonts;
}

const base::flat_set<base::StringPiece>& GetAllowedFontFamilies() {
  return kAllowedFontFamilies;
}

const base::flat_set<base::StringPiece>&
GetAdditionalAllowedFontFamiliesByLocale(WTF::String locale_language) {
  if (locale_language == "ar" || locale_language == "fa" ||
      locale_language == "ur")
    return kAdditionalAllowedFontFamiliesAR;
  if (locale_language == "as")
    return kAdditionalAllowedFontFamiliesAS;
  if (locale_language == "iu")
    return kAdditionalAllowedFontFamiliesIU;
  if (locale_language == "hi" || locale_language == "mr")
    return kAdditionalAllowedFontFamiliesHI;
  if (locale_language == "am" || locale_language == "ti")
    return kAdditionalAllowedFontFamiliesAM;
  if (locale_language == "gu")
    return kAdditionalAllowedFontFamiliesGU;
  if (locale_language == "pa")
    return kAdditionalAllowedFontFamiliesPA;
  if (locale_language == "zh")
    return kAdditionalAllowedFontFamiliesZH;
  if (locale_language == "he")
    return kAdditionalAllowedFontFamiliesHE;
  if (locale_language == "ja")
    return kAdditionalAllowedFontFamiliesJA;
  if (locale_language == "kn")
    return kAdditionalAllowedFontFamiliesKN;
  if (locale_language == "km")
    return kAdditionalAllowedFontFamiliesKM;
  if (locale_language == "ko")
    return kAdditionalAllowedFontFamiliesKO;
  if (locale_language == "lo")
    return kAdditionalAllowedFontFamiliesLO;
  if (locale_language == "ml")
    return kAdditionalAllowedFontFamiliesML;
  return kEmptyFontSet;
}

void set_allowed_font_families_for_testing(
    bool can_restrict_fonts,
    const base::flat_set<base::StringPiece>& allowed_font_families) {
  kCanRestrictFonts = can_restrict_fonts;
  kAllowedFontFamilies = allowed_font_families;
}

}  // namespace brave
