/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_

#include "base/containers/fixed_flat_set.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace ai_chat {

base::span<const webui::LocalizedString> GetLocalizedStrings();

extern const base::fixed_flat_set<std::string_view, 1>
    kPrintPreviewRetrievalHosts;

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
