// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "content/public/browser/web_ui_browser_interface_broker_registry.h"

#include <memory>
#include <ranges>

#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/per_web_ui_browser_interface_broker.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/common/content_client.h"

// Pass |global_binder_initializers_| through to the
// PerWebUIBrowserInterfaceBroker.
// Upstream only passes the initializers for the particular WebUI through, so we
// completely replace the original function. Once
// https://chromium-review.googlesource.com/c/chromium/src/+/7047465 lands we
// can remove this.
#define CreateInterfaceBroker(controller) \
  CreateInterfaceBroker_Chromium(controller)

#include <content/browser/web_ui_browser_interface_broker_registry.cc>

#undef CreateInterfaceBroker

namespace content {

std::unique_ptr<PerWebUIBrowserInterfaceBroker>
WebUIBrowserInterfaceBrokerRegistry::CreateInterfaceBroker(
    WebUIController& controller) {
  auto iter = binder_initializers_.find(controller.GetType());
  if (iter == binder_initializers_.end()) {
    return nullptr;
  }

  std::vector<BinderInitializer> initializers = global_binder_initializers_;
  std::ranges::copy(iter->second, std::back_inserter(initializers));
  return std::make_unique<PerWebUIBrowserInterfaceBroker>(controller,
                                                          initializers);
}

}  // namespace content
