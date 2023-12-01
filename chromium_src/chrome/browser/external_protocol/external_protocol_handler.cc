// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/components/brave_webtorrent/browser/magnet_protocol_handler.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)

#define LaunchUrl LaunchUrl_ChromiumImpl

#include "src/chrome/browser/external_protocol/external_protocol_handler.cc"

#undef LaunchUrl

// static
void ExternalProtocolHandler::LaunchUrl(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    bool is_in_fenced_frame_tree,
    const std::optional<url::Origin>& initiating_origin,
    content::WeakDocumentPtr initiator_document
#if BUILDFLAG(IS_ANDROID)
    ,
    mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory
#endif
) {
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  if (webtorrent::HandleMagnetProtocol(
          url, web_contents_getter, page_transition, has_user_gesture,
          is_in_fenced_frame_tree, initiating_origin, initiator_document)) {
    return;
  }
#endif

  LaunchUrl_ChromiumImpl(url, web_contents_getter, page_transition,
                         has_user_gesture, is_in_fenced_frame_tree,
                         initiating_origin, initiator_document
#if BUILDFLAG(IS_ANDROID)
                         ,
                         out_factory
#endif
  );
}
