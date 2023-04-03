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
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_COMMON_CONSTANTS_H_
