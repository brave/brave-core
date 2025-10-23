// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_UI_BROWSER_INTERFACE_BROKER_REGISTRY_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_UI_BROWSER_INTERFACE_BROKER_REGISTRY_H_

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "content/public/browser/per_web_ui_browser_interface_broker.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

// Adds a method for exposing an interface to all WebUIs in the registry. Once
// https://chromium-review.googlesource.com/c/chromium/src/+/7047465 lands
// upstream we can remove this.
#define AddBinderForTesting                                                 \
  AddBinderForTesting_Unused();                                             \
  template <typename Interface>                                             \
  WebUIBrowserInterfaceBrokerRegistry& AddGlobal(                           \
      base::RepeatingCallback<void(                                         \
          RenderFrameHost*, mojo::PendingReceiver<Interface>)> binder) {    \
    return AddGlobal<Interface>(base::BindRepeating(                        \
        [](base::RepeatingCallback<void(                                    \
               RenderFrameHost*, mojo::PendingReceiver<Interface>)> binder, \
           WebUIController* controller,                                     \
           mojo::PendingReceiver<Interface> receiver) {                     \
          binder.Run(controller->web_ui()->GetRenderFrameHost(),            \
                     std::move(receiver));                                  \
        },                                                                  \
        std::move(binder)));                                                \
  }                                                                         \
  template <typename Interface>                                             \
  WebUIBrowserInterfaceBrokerRegistry& AddGlobal(                           \
      base::RepeatingCallback<void(                                         \
          WebUIController*, mojo::PendingReceiver<Interface>)> binder) {    \
    global_binder_initializers_.push_back(base::BindRepeating(              \
        [](base::RepeatingCallback<void(                                    \
               WebUIController*, mojo::PendingReceiver<Interface>)> binder, \
           WebUIBinderMap* binder_map) {                                    \
          binder_map->Add<Interface>(std::move(binder));                    \
        },                                                                  \
        std::move(binder)));                                                \
    return *this;                                                           \
  }                                                                         \
                                                                            \
 private:                                                                   \
  std::vector<content::BinderInitializer> global_binder_initializers_;      \
                                                                            \
 public:                                                                    \
  template <typename Interface>                                             \
  void AddBinderForTesting

#include <content/public/browser/web_ui_browser_interface_broker_registry.h>  // IWYU pragma: export

#undef AddBinderForTesting

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_UI_BROWSER_INTERFACE_BROKER_REGISTRY_H_
