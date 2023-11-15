/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATION_CONTROLLER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATION_CONTROLLER_IMPL_H_

namespace content {
class BraveNavigationControllerImpl;
}

#define UpdateVirtualURLToURL virtual UpdateVirtualURLToURL
#define session_storage_namespace_map_ \
    session_storage_namespace_map_;    \
    friend class BraveNavigationControllerImpl
#include "src/content/browser/renderer_host/navigation_controller_impl.h"
#undef UpdateVirtualURLToURL
#undef session_storage_namespace_map_

namespace content {

class BraveNavigationControllerImpl : public NavigationControllerImpl {
 public:
  using NavigationControllerImpl::NavigationControllerImpl;

  BraveNavigationControllerImpl(const BraveNavigationControllerImpl&) = delete;
  BraveNavigationControllerImpl& operator=(const BraveNavigationControllerImpl&) = delete;
 private:
  void UpdateVirtualURLToURL(NavigationEntryImpl* entry, const GURL& new_url) override;
};

}

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATION_CONTROLLER_IMPL_H_
