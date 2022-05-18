/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_talk/browser/brave_talk_frame_host.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/components/brave_search/browser/brave_search_default_host.h"
#include "brave/components/brave_talk/common/features.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

namespace brave_talk {

BraveTalkFrameHost::BraveTalkFrameHost(content::WebContents* contents,
                                               const std::string& host)
    : contents_(contents), host_(host) {}

BraveTalkFrameHost::~BraveTalkFrameHost() = default;

void BraveTalkFrameHost::BeginAdvertiseShareDisplayMedia(
    BeginAdvertiseShareDisplayMediaCallback callback) {
  auto* service =
      BraveTalkServiceFactory::GetForContext(contents_->GetBrowserContext());
  service->GetDeviceID(contents_, std::move(callback));
}

}  // namespace brave_talk
