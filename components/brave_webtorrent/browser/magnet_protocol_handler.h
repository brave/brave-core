/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_MAGNET_PROTOCOL_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_MAGNET_PROTOCOL_HANDLER_H_

#include <optional>

#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#include "content/public/browser/weak_document_ptr.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

// Define this variable in the url namespace to make patching easier.
namespace url {

inline constexpr char kWebTorrentScheme[] = "webtorrent";

}  // namespace url

namespace webtorrent {

GURL TranslateMagnetURL(const GURL& url);

GURL TranslateTorrentUIURLReversed(const GURL& url);
bool HandleTorrentURLReverseRewrite(GURL* url,
                                    content::BrowserContext* browser_context);

bool HandleTorrentURLRewrite(GURL* url,
                             content::BrowserContext* browser_context);
bool HandleMagnetURLRewrite(GURL* url,
                            content::BrowserContext* browser_context);

bool HandleMagnetProtocol(const GURL& url,
                          content::WebContents::Getter web_contents_getter,
                          ui::PageTransition page_transition,
                          bool has_user_gesture,
                          bool is_in_fenced_frame_tree,
                          const std::optional<url::Origin>& initiating_origin,
                          content::WeakDocumentPtr initiator_document);

}  // namespace webtorrent

#endif  // BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_MAGNET_PROTOCOL_HANDLER_H_
