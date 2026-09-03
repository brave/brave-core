/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_plain_video_detector.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/token.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace playlist {

// static
void PlaylistPlainVideoDetector::CreateForWebContents(
    content::WebContents* web_contents,
    MediaDetectedCallback callback) {
  content::WebContentsUserData<PlaylistPlainVideoDetector>::
      CreateForWebContents(web_contents, std::move(callback));
}

PlaylistPlainVideoDetector::PlaylistPlainVideoDetector(
    content::WebContents* web_contents,
    MediaDetectedCallback callback)
    : content::WebContentsUserData<PlaylistPlainVideoDetector>(*web_contents),
      content::WebContentsObserver(web_contents),
      network_observer_(PlaylistNetworkObserver::GetOrCreate(
          web_contents->GetBrowserContext())),
      callback_(std::move(callback)) {
  network_observer_->AddObserver(this);
}

PlaylistPlainVideoDetector::~PlaylistPlainVideoDetector() {
  network_observer_->RemoveObserver(this);
}

void PlaylistPlainVideoDetector::OnMediaResponseObserved(
    const content::GlobalRenderFrameHostToken& frame_token,
    const MediaResponseInfo& info) {
  // kProgressive is exactly the plain <video>/<audio src> case: a whole file
  // requested by the media element itself. Manifests and MSE fragments
  // (kHlsManifest/kDashManifest/kSegment) are out of scope for this patch.
  if (info.kind != MediaKind::kProgressive) {
    return;
  }

  // The network observer is per-BrowserContext, so most responses belong to
  // other tabs.
  auto* render_frame_host =
      content::RenderFrameHost::FromFrameToken(frame_token);
  if (!render_frame_host || content::WebContents::FromRenderFrameHost(
                                render_frame_host) != web_contents()) {
    return;
  }

  if (!notified_urls_.insert(info.url).second) {
    return;
  }

  const GURL page_url = web_contents()->GetLastCommittedURL();

  auto item = mojom::PlaylistItem::New();
  item->id = base::Token::CreateRandom().ToString();
  item->page_source = page_url;
  item->page_redirected = page_url;
  item->media_source = info.url;
  item->media_path = info.url;
  item->is_blob_from_media_source = false;
  item->name = base::UTF16ToUTF8(web_contents()->GetTitle());

  std::vector<mojom::PlaylistItemPtr> items;
  items.push_back(std::move(item));
  callback_.Run(page_url, std::move(items));
}

}  // namespace playlist
