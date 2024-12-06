/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/offline_pages/background_loader_offliner.h"

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "chrome/browser/content_settings/page_specific_content_settings_delegate.h"
#include "components/content_settings/browser/page_specific_content_settings.h"

#define PageSpecificContentSettings                                     \
  PageSpecificContentSettings* unused_pscs [[maybe_unused]];            \
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents( \
      loader_->web_contents());                                         \
  content_settings::PageSpecificContentSettings

#include "src/chrome/browser/offline_pages/background_loader_offliner.cc"

#undef PageSpecificContentSettings
