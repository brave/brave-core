// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_

#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace speedreader {

constexpr const int kIsolatedWorldId = ISOLATED_WORLD_ID_SPEEDREADER;

constexpr webui::LocalizedString kLocalizedStrings[] = {
    {"braveSpeedreader", IDS_SPEEDREADER_BRAND_LABEL_2},
    {"braveSpeedreaderAlwaysLoadLabel", IDS_SPEEDREADER_ALWAYS_LOAD_LABEL},
    {"braveSpeedreaderThemeLabel", IDS_SPEEDREADER_THEME_LABEL},
    {"braveSpeedreaderFontStyleLabel", IDS_SPEEDREADER_FONT_STYLE_LABEL},
    {"braveSpeedreaderContentStyleLabel", IDS_SPEEDREADER_CONTENT_STYLE_LABEL},
    {"braveSpeedreaderTurnOffDesc", IDS_SPEEDREADER_TURN_OFF_DESC},
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
