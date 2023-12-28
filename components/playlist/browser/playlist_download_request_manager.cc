/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_download_request_manager.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/playlist/common/features.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/isolated_world_ids.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/page_transition_types.h"

namespace playlist {

PlaylistDownloadRequestManager::Request::Request() = default;
PlaylistDownloadRequestManager::Request&
PlaylistDownloadRequestManager::Request::operator=(
    PlaylistDownloadRequestManager::Request&&) noexcept = default;
PlaylistDownloadRequestManager::Request::Request(
    PlaylistDownloadRequestManager::Request&&) noexcept = default;
PlaylistDownloadRequestManager::Request::~Request() = default;

PlaylistDownloadRequestManager::PlaylistDownloadRequestManager(
    PlaylistService* service,
    content::BrowserContext* context,
    MediaDetectorComponentManager* manager)
    : service_(service),
      context_(context),
      media_detector_component_manager_(manager) {}

PlaylistDownloadRequestManager::~PlaylistDownloadRequestManager() = default;

void PlaylistDownloadRequestManager::CreateWebContents(
    bool should_force_fake_ua) {
  content::WebContents::CreateParams create_params(context_, nullptr);
  create_params.is_never_visible = true;
  web_contents_ = content::WebContents::Create(create_params);
  web_contents_->SetAudioMuted(true);
  PlaylistTabHelper::MaybeCreateForWebContents(web_contents_.get(),
                                               service_.get());
  if (should_force_fake_ua ||
      base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    DVLOG(2) << __func__ << " Faked UA to detect media files";
    blink::UserAgentOverride user_agent(
        "Mozilla/5.0 (iPhone; CPU iPhone OS 13_2_3 like Mac OS X) "
        "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.3 "
        "Mobile/15E148 "
        "Safari/604.1",
        /* user_agent_metadata */ {});
    web_contents_->SetUserAgentOverride(user_agent,
                                        /* override_in_new_tabs= */ true);
  }
}

void PlaylistDownloadRequestManager::GetMediaFilesFromPage(Request request) {
  DVLOG(2) << __func__;
  if (!ReadyToRunMediaDetectorScript()) {
    if (!request_start_time_.is_null()) {
      // See if the last job is stuck.
#if DCHECK_IS_ON()
      DCHECK(base::Time::Now() - request_start_time_ <= base::Minutes(1));
#else
      if (base::Time::Now() - request_start_time_ > base::Minutes(1)) {
        LOG(ERROR) << "The previous job is pending longer than 1 min";
      }
#endif
    }

    pending_requests_.push_back(std::move(request));
    DVLOG(2) << "Queued request";
    return;
  }

  RunMediaDetector(std::move(request));
}

void PlaylistDownloadRequestManager::FetchPendingRequest() {
  if (pending_requests_.empty() || !ReadyToRunMediaDetectorScript())
    return;

  auto request = std::move(pending_requests_.front());
  pending_requests_.pop_front();
  RunMediaDetector(std::move(request));
}

void PlaylistDownloadRequestManager::RunMediaDetector(Request request) {
  DVLOG(2) << __func__;

  DCHECK_GE(in_progress_urls_count_, 0);
  in_progress_urls_count_++;

  DCHECK(callback_for_current_request_.is_null());

  callback_for_current_request_ = std::move(request.callback);
  DCHECK(callback_for_current_request_)
      << "Empty callback shouldn't be requested";
  request_start_time_ = base::Time::Now();

  // Start to request on clean slate, so that result won't be affected by
  // previous page.
  CreateWebContents(request.should_force_fake_ua);

  DCHECK(request.url.is_valid());
  DCHECK(web_contents_);
  DVLOG(2) << "Load URL to detect media files: " << request.url.spec();
  auto load_url_params =
      content::NavigationController::LoadURLParams(request.url);
  if (base::FeatureList::IsEnabled(features::kPlaylistFakeUA) ||
      request.should_force_fake_ua) {
    load_url_params.override_user_agent =
        content::NavigationController::UA_OVERRIDE_TRUE;
  }

  content::NavigationController& controller = web_contents_->GetController();
  controller.LoadURLWithParams(load_url_params);

  if (base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    for (int i = 0; i < controller.GetEntryCount(); ++i) {
      controller.GetEntryAtIndex(i)->SetIsOverridingUserAgent(true);
    }
  }
}

bool PlaylistDownloadRequestManager::ReadyToRunMediaDetectorScript() const {
  return in_progress_urls_count_ == 0;
}

std::vector<mojom::PlaylistItemPtr>
PlaylistDownloadRequestManager::ProcessFoundMedia(
    content::WebContents* contents,
    const GURL& url,
    base::Value value) {
  CHECK(contents);

  /* Expected output:
    [
      {
        "detected": boolean,
        "mimeType": "video" | "audio",
        "name": string,
        "pageSrc": url,
        "pageTitle": string
        "src": url
        "srcIsMediaSourceObjectURL": boolean,
        "thumbnail": url | undefined
      }
    ]
  */

  if (value.is_dict() && value.GetDict().empty()) {
    DVLOG(2) << "No media was detected";
    return {};
  }

  CHECK(value.is_list()) << " Got invalid value after running media detector "
                            "script: Should be list";

  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto& media : value.GetList()) {
    if (!media.is_dict()) {
      LOG(ERROR) << __func__ << " Got invalid item";
      continue;
    }

    const auto& media_dict = media.GetDict();

    auto* name = media_dict.FindString("name");
    auto* page_title = media_dict.FindString("pageTitle");
    auto* page_source = media_dict.FindString("pageSrc");
    auto* mime_type = media_dict.FindString("mimeType");
    auto* src = media_dict.FindString("src");
    auto is_blob_from_media_source =
        media_dict.FindBool("srcIsMediaSourceObjectURL");
    if (!name || !page_source || !page_title || !mime_type || !src ||
        !is_blob_from_media_source) {
      LOG(ERROR) << __func__ << " required fields are not satisfied";
      continue;
    }

    // nullable data
    auto* thumbnail = media_dict.FindString("thumbnail");
    auto* author = media_dict.FindString("author");
    auto duration = media_dict.FindDouble("duration");

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->page_source = url;
    item->page_redirected = GURL(*page_source);
    item->name = *name;
    // URL data
    if (GURL media_url(*src);
        !media_url.SchemeIs(url::kHttpsScheme) && !media_url.SchemeIsBlob()) {
      continue;
    }

    item->media_source = GURL(*src);
    item->media_path = GURL(*src);
    item->is_blob_from_media_source = *is_blob_from_media_source;
    if (!CanCacheMedia(item)) {
      LOG(ERROR)
          << __func__
          << "media scheme is not https:// nor blob: that we can cache from";
      continue;
    }

    if (thumbnail) {
      if (GURL thumbnail_url(*thumbnail);
          !thumbnail_url.SchemeIs(url::kHttpsScheme)) {
        LOG(ERROR) << __func__ << "thumbnail scheme is not https://";
        thumbnail = nullptr;
      }
    }

    if (duration.has_value()) {
      item->duration =
          base::TimeDeltaToValue(base::Seconds(*duration)).GetString();
    }
    if (thumbnail) {
      item->thumbnail_source = GURL(*thumbnail);
      item->thumbnail_path = GURL(*thumbnail);
    }
    if (author) {
      item->author = *author;
    }
    items.push_back(std::move(item));
  }

  DVLOG(2) << __func__ << " Media detection result size: " << items.size();

  if (contents == background_contents() && items.size()) {
    CHECK(!callback_for_current_request_.is_null()) << " callback already ran";
    auto callback = std::move(callback_for_current_request_);

    DCHECK_GT(in_progress_urls_count_, 0);
    in_progress_urls_count_--;

    std::vector<mojom::PlaylistItemPtr> cloned_items;
    base::ranges::transform(items, std::back_inserter(cloned_items),
                            &mojom::PlaylistItemPtr::Clone);
    std::move(callback).Run(std::move(cloned_items));

    web_contents_.reset();
  }

  return items;
}

bool PlaylistDownloadRequestManager::CanCacheMedia(
    const mojom::PlaylistItemPtr& item) const {
  GURL media_url(item->media_source);
  if (media_url.SchemeIs(url::kHttpsScheme)) {
    return true;
  }

  if (media_url.SchemeIsBlob()) {
    if (item->is_blob_from_media_source) {
      // At this moment, we have a few sites that we can get media files with
      // hacks.
      return media_detector_component_manager_->ShouldHideMediaSrcAPI(
                 media_url) ||
             media_detector_component_manager_->ShouldUseFakeUA(media_url);
    }

    // blob: which is not Media Source
    // TODO(sko) Test and allow this case referring to
    // https://github.com/brave/brave-core/pull/17246
    return false;
  }

  return false;
}

bool PlaylistDownloadRequestManager::ShouldRefetchMediaSourceToCache(
    const mojom::PlaylistItemPtr& item) const {
  GURL media_url(item->media_source);
  if (media_url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  if (media_url.SchemeIsBlob() && item->is_blob_from_media_source) {
    CHECK(media_detector_component_manager_->ShouldHideMediaSrcAPI(media_url) ||
          media_detector_component_manager_->ShouldUseFakeUA(media_url));
    return true;
  }

  NOTREACHED_NORETURN()
      << "CanCacheMedia() should be true when this method is called";
}

// static
void PlaylistDownloadRequestManager::SetRunScriptOnMainWorldForTest() {
  CHECK_IS_TEST();
  s_run_script_on_main_world = true;
}

// static
bool PlaylistDownloadRequestManager::s_run_script_on_main_world = false;

void PlaylistDownloadRequestManager::ConfigureWebPrefsForBackgroundWebContents(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* web_prefs) {
  web_prefs->should_detect_media_files = true;
  for (const auto& [site, script] :
       media_detector_component_manager_->GetAllMediaDetectorScripts()) {
    web_prefs->url_and_media_detection_scripts.insert(
        {site.Serialize(), script});
  }

  if (web_contents_ && web_contents_.get() == web_contents) {
    // Background web contents.
    web_prefs->force_cosmetic_filtering = true;
    web_prefs->hide_media_src_api = true;
  }
#if BUILDFLAG(IS_ANDROID)
  // We need this to get metadata from js object from certain sites. As calling
  // AllowJavascript() is allowed on Android and we're already calling it from
  // other places, this should be fine.
  // https://github.com/brave/reviews/issues/1151
  web_prefs->allow_to_run_script_on_main_world = true;
#endif
  if (s_run_script_on_main_world) {
    web_prefs->allow_to_run_script_on_main_world = true;
  }
}

void PlaylistDownloadRequestManager::ResetRequests() {
  if (web_contents_) {
    web_contents_.reset();
  }

  pending_requests_.clear();
  request_start_time_ = {};
  in_progress_urls_count_ = 0;
  callback_for_current_request_ = base::NullCallback();
}

content::WebContents*
PlaylistDownloadRequestManager::GetBackgroundWebContentsForTesting() {
  if (!web_contents_) {
    CreateWebContents(false);
  }

  return web_contents_.get();
}

}  // namespace playlist
