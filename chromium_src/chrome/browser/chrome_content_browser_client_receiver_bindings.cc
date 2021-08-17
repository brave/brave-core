/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_content_browser_client.h"

#include "brave/browser/brave_drm_tab_helper.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"

// ChromeContentBrowserClient::BindAssociatedReceiverFromFrame() overrides
// a method from content::ContentBrowserClient() so we use a one-line C++
// patch here to avoid having to override several .cc and .h files.
#define BRAVE_BIND_ASSOCIATED_RECEIVER_FROM_FRAME                         \
  if (interface_name == brave_drm::mojom::BraveDRM::Name_) {              \
    BraveDrmTabHelper::BindBraveDRM(                                      \
        mojo::PendingAssociatedReceiver<brave_drm::mojom::BraveDRM>(      \
            std::move(*handle)),                                          \
        render_frame_host);                                               \
    return true;                                                          \
  }                                                                       \
  if (interface_name == brave_shields::mojom::BraveShieldsHost::Name_) {  \
    brave_shields::BraveShieldsWebContentsObserver::BindBraveShieldsHost( \
        mojo::PendingAssociatedReceiver<                                  \
            brave_shields::mojom::BraveShieldsHost>(std::move(*handle)),  \
        render_frame_host);                                               \
    return true;                                                          \
  }

#include "../../../../chrome/browser/chrome_content_browser_client_receiver_bindings.cc"  // NOLINT

#undef BRAVE_BIND_ASSOCIATED_RECEIVER_FROM_FRAME
