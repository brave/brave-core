/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/navigation_controller_impl.h"

#include "src/content/browser/renderer_host/navigation_controller_impl.cc"

namespace content {

void BraveNavigationControllerImpl::UpdateVirtualURLToURL(
    NavigationEntryImpl* entry,
    const GURL& new_url) {
  NavigationControllerImpl::UpdateVirtualURLToURL(entry, new_url);

  GURL override_virtual_url = entry->GetVirtualURL();
  if (override_virtual_url.SchemeIs(content::kChromeUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    override_virtual_url = override_virtual_url.ReplaceComponents(replacements);
    entry->SetVirtualURL(override_virtual_url);
  }
}

}  // namespace content
