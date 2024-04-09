/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_web_contentses.h"

#include <string>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/no_destructor.h"
#include "brave/components/playlist/browser/playlist_background_web_contents_helper.h"
#include "brave/components/playlist/common/features.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "net/base/schemeful_site.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"

namespace {
const char* GetUserAgentOverride(const GURL& url) {
  // iPhone iOS 13.2.3 - Safari 13.0.3
  static constexpr char kUserAgentOverride[] =
      "Mozilla/5.0 (iPhone; CPU iPhone OS 13_2_3 like Mac OS X) "
      "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.3 "
      "Mobile/15E148 "
      "Safari/604.1";

  if (base::FeatureList::IsEnabled(playlist::features::kPlaylistFakeUA)) {
    return kUserAgentOverride;
  }

  static const base::NoDestructor<base::flat_set<net::SchemefulSite>> kSites{
      {net::SchemefulSite(GURL("https://ted.com"))}};

  return kSites->contains(net::SchemefulSite(url)) ? kUserAgentOverride
                                                   : nullptr;
}
}  // namespace

namespace playlist {

PlaylistBackgroundWebContentses::PlaylistBackgroundWebContentses(
    content::BrowserContext* context,
    PlaylistService* service)
    : context_(context), service_(service) {}

PlaylistBackgroundWebContentses::~PlaylistBackgroundWebContentses() = default;

void PlaylistBackgroundWebContentses::Add(
    const GURL& url,
    PlaylistMediaHandler::OnceCallback on_media_detected_callback,
    base::TimeDelta timeout) {
  auto web_contents = content::WebContents::Create(
      content::WebContents::CreateParams(context_));
  web_contents->SetAudioMuted(true);

  auto [callback_for_media_handler, callback_for_timer] =
      base::SplitOnceCallback(base::BindOnce(
          &PlaylistBackgroundWebContentses::Remove, weak_factory_.GetWeakPtr(),
          web_contents.get(), std::move(on_media_detected_callback)));

  PlaylistBackgroundWebContentsHelper::CreateForWebContents(
      web_contents.get(), service_.get(),
      std::move(callback_for_media_handler));

  auto load_url_params = content::NavigationController::LoadURLParams(url);
  if (auto* ua_override = GetUserAgentOverride(url)) {
    web_contents->SetUserAgentOverride(
        blink::UserAgentOverride::UserAgentOnly(ua_override),
        /* override_in_new_tabs = */ true);
    load_url_params.override_user_agent =
        content::NavigationController::UA_OVERRIDE_TRUE;
  }
  web_contents->GetController().LoadURLWithParams(load_url_params);

  background_web_contentses_[std::move(web_contents)].Start(
      FROM_HERE, timeout,
      base::BindOnce(std::move(callback_for_timer), GURL(),
                     std::vector<mojom::PlaylistItemPtr>()));
}

void PlaylistBackgroundWebContentses::Reset() {
  background_web_contentses_.clear();
}

void PlaylistBackgroundWebContentses::Remove(
    content::WebContents* web_contents,
    PlaylistMediaHandler::OnceCallback on_media_detected_callback,
    GURL url,
    std::vector<mojom::PlaylistItemPtr> items) {
  const auto it = background_web_contentses_.find(web_contents);
  CHECK(it != background_web_contentses_.cend());
  it->second.Stop();  // no-op if called by the timer
  background_web_contentses_.erase(it);

  std::move(on_media_detected_callback).Run(std::move(url), std::move(items));
}

}  // namespace playlist
