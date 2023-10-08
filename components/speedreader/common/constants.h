// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_

#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace speedreader {

constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"braveReaderModeCaption", IDS_READER_MODE_CAPTION},
    {"braveReaderModeClose", IDS_READER_MODE_CLOSE},
    {"braveReaderModeExit", IDS_READER_MODE_EXIT},
    {"braveReaderModeVoice", IDS_READER_MODE_VOICE},
    {"braveReaderModeTune", IDS_READER_MODE_TUNE},
    {"braveReaderModeAppearance", IDS_READER_MODE_APPEARANCE},
    {"braveReaderModeAppearanceThemeLight",
     IDS_READER_MODE_APPEARANCE_THEME_LIGHT},
    {"braveReaderModeAppearanceThemeSepia",
     IDS_READER_MODE_APPEARANCE_THEME_SEPIA},
    {"braveReaderModeAppearanceThemeDark",
     IDS_READER_MODE_APPEARANCE_THEME_DARK},
    {"braveReaderModeAppearanceThemeSystem",
     IDS_READER_MODE_APPEARANCE_THEME_SYSTEM},
    {"braveReaderModeAppearanceFontSans", IDS_READER_MODE_APPEARANCE_FONT_SANS},
    {"braveReaderModeAppearanceFontSerif",
     IDS_READER_MODE_APPEARANCE_FONT_SERIF},
    {"braveReaderModeAppearanceFontMono", IDS_READER_MODE_APPEARANCE_FONT_MONO},
    {"braveReaderModeAppearanceFontDyslexic",
     IDS_READER_MODE_APPEARANCE_FONT_DYSLEXIC},
    {"braveReaderModeAppearanceColumnWidthNarrow",
     IDS_READER_MODE_APPEARANCE_COLUMN_WIDTH_NARROW},
    {"braveReaderModeAppearanceColumnWidthWide",
     IDS_READER_MODE_APPEARANCE_COLUMN_WIDTH_WIDE},
    {"braveReaderModeTextToSpeech", IDS_READER_MODE_TEXT_TO_SPEECH},
    {"braveReaderModeAI", IDS_READER_MODE_AI},
    {"braveReaderModeFontSizeDecrease",
     IDS_READER_MODE_APPEARANCE_FONT_SIZE_DECREASE},
    {"braveReaderModeFontSizeIncrease",
     IDS_READER_MODE_APPEARANCE_FONT_SIZE_INCREASE},
    {"braveReaderModeTtsRewind", IDS_READER_MODE_TEXT_TO_SPEECH_REWIND},
    {"braveReaderModeTtsPlayPause", IDS_READER_MODE_TEXT_TO_SPEECH_PLAY_PAUSE},
    {"braveReaderModeTtsForward", IDS_READER_MODE_TEXT_TO_SPEECH_FORWARD},
    {"braveReaderModeTtsSpeedDecrease",
     IDS_READER_MODE_TEXT_TO_SPEECH_SPEED_DECREASE},
    {"braveReaderModeTtsSpeedIncrease",
     IDS_READER_MODE_TEXT_TO_SPEECH_SPEED_INCREASE},
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
