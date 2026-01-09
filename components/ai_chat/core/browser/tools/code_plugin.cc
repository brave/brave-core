// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/code_plugin.h"

namespace ai_chat {

CodePlugin::~CodePlugin() = default;

std::optional<std::string> CodePlugin::ValidateOutput(
    const base::Value::Dict& output) const {
  return std::nullopt;
}

}  // namespace ai_chat
