// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/test/ad_block_service_test_observer.h"

namespace brave_shields {

AdBlockServiceTestObserver::AdBlockServiceTestObserver(
    AdBlockService* service) {
  observation_.Observe(service);
}

AdBlockServiceTestObserver::~AdBlockServiceTestObserver() = default;

void AdBlockServiceTestObserver::WaitForDefault() {
  default_loop_.Run();
}

void AdBlockServiceTestObserver::WaitForAdditional() {
  additional_loop_.Run();
}

void AdBlockServiceTestObserver::Wait(bool is_default_engine) {
  if (is_default_engine) {
    WaitForDefault();
  } else {
    WaitForAdditional();
  }
}

void AdBlockServiceTestObserver::OnFilterListLoaded(
    bool is_default_engine,
    AdBlockService::FilterListLoadResult result) {
  (is_default_engine ? default_loop_ : additional_loop_).Quit();
}

}  // namespace brave_shields
