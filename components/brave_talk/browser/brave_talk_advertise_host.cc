#include "brave/components/brave_talk/browser/brave_talk_advertise_host.h"

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/brave_search_default_host.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "brave/components/brave_talk/common/features.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_talk {

BraveTalkAdvertiseHost::BraveTalkAdvertiseHost(
    const std::string& host)
    : host_(host) {}

BraveTalkAdvertiseHost::~BraveTalkAdvertiseHost() = default;

void BraveTalkAdvertiseHost::BeginAdvertiseShareDisplayMedia(BeginAdvertiseShareDisplayMediaCallback callback) {
    std::move(callback).Run("hello world");
}

}  // namespace brave_search
