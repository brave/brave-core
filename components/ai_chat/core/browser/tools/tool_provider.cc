// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"

namespace ai_chat {

ToolProvider::ToolProvider() = default;

ToolProvider::~ToolProvider() = default;

bool ToolProvider::IsPausedByUser() {
  return false;
}

void ToolProvider::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ToolProvider::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void ToolProvider::NotifyTaskStateChanged() {
  for (auto& observer : observers_) {
    observer.OnTaskStateChanged();
  }
}

}  // namespace ai_chat
