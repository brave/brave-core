/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_webcontents.h"

#include <set>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/time/time.h"
#include "brave/components/playlist/browser/playlist_background_webcontents_helper.h"
#include "brave/components/playlist/common/features.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "net/base/schemeful_site.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "url/gurl.h"

namespace {
const char* GetUserAgentOverride(const GURL& url) {
  static constexpr char kUserAgentOverride[] =
      "Mozilla/5.0 (iPhone; CPU iPhone OS 13_2_3 like Mac OS X) "
      "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.3 "
      "Mobile/15E148 "
      "Safari/604.1";

  if (base::FeatureList::IsEnabled(playlist::features::kPlaylistFakeUA)) {
    return kUserAgentOverride;
  }

  static const std::set kSites{net::SchemefulSite(GURL("https://ted.com"))};

  return kSites.contains(net::SchemefulSite(url)) ? kUserAgentOverride
                                                  : nullptr;
}
}  // namespace

namespace playlist {

PlaylistBackgroundWebContents::PlaylistBackgroundWebContents(
    content::BrowserContext* context,
    PlaylistService* service)
    : context_(context), service_(service) {}

PlaylistBackgroundWebContents::~PlaylistBackgroundWebContents() = default;

void PlaylistBackgroundWebContents::Add(
    const GURL& url,
    PlaylistMediaHandler::OnceCallback on_media_detected_callback) {
  content::WebContents::CreateParams create_params(context_);
  create_params.is_never_visible = true;

  auto web_contents = content::WebContents::Create(create_params);
  web_contents->SetAudioMuted(true);

  auto [callback_for_media_handler, callback_for_timer] =
      base::SplitOnceCallback(base::BindOnce(
          &PlaylistBackgroundWebContents::Remove, weak_factory_.GetWeakPtr(),
          web_contents.get(), std::move(on_media_detected_callback)));

  PlaylistMediaHandler::CreateForWebContents(
      web_contents.get(), std::move(callback_for_media_handler));
  PlaylistBackgroundWebContentsHelper::CreateForWebContents(web_contents.get(),
                                                            service_);

  auto load_url_params = content::NavigationController::LoadURLParams(url);
  if (auto* ua_override = GetUserAgentOverride(url)) {
    web_contents->SetUserAgentOverride(
        blink::UserAgentOverride::UserAgentOnly(ua_override),
        /* override_in_new_tabs = */ true);
    load_url_params.override_user_agent =
        content::NavigationController::UA_OVERRIDE_TRUE;
  }
  web_contents->GetController().LoadURLWithParams(load_url_params);

  background_web_contents_[std::move(web_contents)].Start(
      FROM_HERE, base::Seconds(10),
      base::BindOnce(std::move(callback_for_timer),
                     std::vector<mojom::PlaylistItemPtr>(), GURL()));
}

void PlaylistBackgroundWebContents::Remove(
    content::WebContents* web_contents,
    PlaylistMediaHandler::OnceCallback on_media_detected_callback,
    std::vector<mojom::PlaylistItemPtr> items,
    const GURL& url) {
  const auto it = background_web_contents_.find(web_contents);
  CHECK(it != background_web_contents_.cend());
  it->second.Stop();  // no-op if called by the timer
  background_web_contents_.erase(it);

  std::move(on_media_detected_callback).Run(std::move(items), url);
}

}  // namespace playlist
