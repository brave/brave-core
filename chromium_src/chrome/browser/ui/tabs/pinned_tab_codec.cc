/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/pinned_tab_codec.h"

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_specifier_utils.h"
#endif

namespace {

void BraveModifyEncodedPinnedTab(content::WebContents* web_contents,
                                 base::Value& value) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  containers::SerializeContainerSpecifier(web_contents, value);
#endif
}

void BraveModifyDecodedPinnedTab(const base::Value& value, StartupTab& tab) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  containers::DeserializeContainerSpecifier(value, tab.container);
#endif
}

}  // namespace

#include <chrome/browser/ui/tabs/pinned_tab_codec.cc>
