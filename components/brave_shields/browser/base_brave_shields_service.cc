/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/base_brave_shields_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace brave_shields {

BaseBraveShieldsService::BaseBraveShieldsService(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : initialized_(false), task_runner_(task_runner) {}

BaseBraveShieldsService::~BaseBraveShieldsService() = default;

bool BaseBraveShieldsService::IsInitialized() const {
  return initialized_;
}

void BaseBraveShieldsService::InitShields() {
  if (Init()) {
    std::lock_guard<std::mutex> guard(initialized_mutex_);
    initialized_ = true;
  }
}

bool BaseBraveShieldsService::Start() {
  if (initialized_) {
    return true;
  }

  InitShields();
  return false;
}

void BaseBraveShieldsService::ShouldStartRequest(
    const GURL& url,
    blink::mojom::ResourceType resource_type,
    const std::string& tab_host,
    bool aggressive_blocking,
    bool* did_match_rule,
    bool* did_match_exception,
    bool* did_match_important,
    std::string* mock_data_url) {}

scoped_refptr<base::SequencedTaskRunner>
BaseBraveShieldsService::GetTaskRunner() {
  return task_runner_;
}

}  // namespace brave_shields
