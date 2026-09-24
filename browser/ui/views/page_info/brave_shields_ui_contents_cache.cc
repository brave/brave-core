/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_shields_ui_contents_cache.h"

#include "base/check.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"

namespace {

// Duration after which the cached Shields WebUI contents wrapper expires.
constexpr auto kCacheExpiryInterval = base::Seconds(30);

}  // namespace

BraveShieldsUIContentsCache::BraveShieldsUIContentsCache()
    : cache_timer_(std::make_unique<base::RetainingOneShotTimer>(
          FROM_HERE,
          kCacheExpiryInterval,
          base::BindRepeating(
              &BraveShieldsUIContentsCache::ResetCachedShieldsUIContents,
              base::Unretained(this)))) {}

BraveShieldsUIContentsCache::~BraveShieldsUIContentsCache() = default;

std::unique_ptr<WebUIContentsWrapper>
BraveShieldsUIContentsCache::GetCachedShieldsUIContents() {
  cache_timer_->Stop();
  return std::move(contents_wrapper_);
}

void BraveShieldsUIContentsCache::CacheShieldsUIContents(
    std::unique_ptr<WebUIContentsWrapper> contents_wrapper) {
  contents_wrapper_ = std::move(contents_wrapper);
  if (contents_wrapper_) {
    cache_timer_->Reset();
  }
}

void BraveShieldsUIContentsCache::ResetCachedShieldsUIContents() {
  CacheShieldsUIContents(nullptr);
}
