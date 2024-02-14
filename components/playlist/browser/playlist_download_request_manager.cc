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
#include "brave/components/playlist/browser/playlist_service.h"
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

namespace {

constexpr int32_t kInvalidWorldID = -1;

int32_t g_playlist_javascript_world_id = kInvalidWorldID;
bool g_run_script_on_main_world = false;

bool PlaylistJavaScriptWorldIdIsSet() {
  return g_playlist_javascript_world_id != kInvalidWorldID;
}

}  // namespace

PlaylistDownloadRequestManager::Request::Request() = default;
PlaylistDownloadRequestManager::Request&
PlaylistDownloadRequestManager::Request::operator=(
    PlaylistDownloadRequestManager::Request&&) noexcept = default;
PlaylistDownloadRequestManager::Request::Request(
    PlaylistDownloadRequestManager::Request&&) noexcept = default;
PlaylistDownloadRequestManager::Request::~Request() = default;

// static
void PlaylistDownloadRequestManager::SetPlaylistJavaScriptWorldId(
    const int32_t id) {
  // Never allow running in main world (0).
  CHECK(id > content::ISOLATED_WORLD_ID_CONTENT_END);
  // Only allow ID to be set once.
  if (PlaylistJavaScriptWorldIdIsSet()) {
    CHECK_IS_TEST();
  }
  g_playlist_javascript_world_id = id;
}

// static
void PlaylistDownloadRequestManager::SetRunScriptOnMainWorldForTest() {
  CHECK_IS_TEST();
  g_run_script_on_main_world = true;
}

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
  if (pending_requests_.empty() || !ReadyToRunMediaDetectorScript()) {
    return;
  }

  auto request = std::move(pending_requests_.front());
  pending_requests_.pop_front();
  RunMediaDetector(std::move(request));
}

void PlaylistDownloadRequestManager::RunMediaDetector(Request request) {
  DVLOG(2) << __func__;
  CHECK(PlaylistJavaScriptWorldIdIsSet());

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

void PlaylistDownloadRequestManager::GetMedia(
    content::WebContents* contents,
    base::OnceCallback<void(std::vector<mojom::PlaylistItemPtr>)> cb) {
  DVLOG(2) << __func__;
  CHECK(contents && contents->GetPrimaryMainFrame());

  const auto& media_detector_script =
      media_detector_component_manager_->GetMediaDetectorScript(
          contents->GetLastCommittedURL());
  DCHECK(!media_detector_script.empty());

  auto callback = base::BindOnce(
      &PlaylistDownloadRequestManager::OnGetMedia, weak_factory_.GetWeakPtr(),
      contents->GetWeakPtr(), contents->GetLastCommittedURL(), std::move(cb));

#if BUILDFLAG(IS_ANDROID)
  content::RenderFrameHost::AllowInjectingJavaScript();
  PlaylistTabHelper::FromWebContents(contents)->RequestAsyncExecuteScript(
      content::ISOLATED_WORLD_ID_GLOBAL /* main_world*/,
      base::UTF8ToUTF16(media_detector_script), std::move(callback));
#else
  if (g_run_script_on_main_world) {
    PlaylistTabHelper::FromWebContents(contents)->RequestAsyncExecuteScript(
        content::ISOLATED_WORLD_ID_GLOBAL /* main_world*/,
        base::UTF8ToUTF16(media_detector_script), std::move(callback));
  } else {
    CHECK(PlaylistJavaScriptWorldIdIsSet());
    PlaylistTabHelper::FromWebContents(contents)->RequestAsyncExecuteScript(
        g_playlist_javascript_world_id,
        base::UTF8ToUTF16(media_detector_script), std::move(callback));
  }
#endif
}

void PlaylistDownloadRequestManager::OnGetMedia(
    base::WeakPtr<content::WebContents> contents,
    GURL url,
    base::OnceCallback<void(std::vector<mojom::PlaylistItemPtr>)> cb,
    base::Value value) {
  if (!contents) {
    return;
  }

  DVLOG(2) << __func__;

  auto items = ProcessFoundMedia(std::move(value), url);

  if (contents.get() == background_contents() && items.size()) {
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

  std::move(cb).Run(std::move(items));

  FetchPendingRequest();
}

std::vector<mojom::PlaylistItemPtr>
PlaylistDownloadRequestManager::ProcessFoundMedia(base::Value value,
                                                  GURL page_url) {
  /* Expected output:
    [
      {
        "mimeType": "video" | "audio",
        "name": string,
        "pageSrc": url,
        "pageTitle": string
        "src": url
        "srcIsMediaSourceObjectURL": boolean,
        "thumbnail": url | undefined
        "duration": double | undefined
        "author": string | undefined
      }
    ]
  */

  std::vector<mojom::PlaylistItemPtr> items;
  if (value.is_dict() && value.GetDict().empty()) {
    DVLOG(2) << "No media was detected";
    return items;
  }

  CHECK(value.is_list()) << " Got invalid value after running media detector "
                            "script: Should be list";
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
    item->page_source = page_url;
    item->page_redirected = GURL(*page_source);
    item->name = *name;
    // URL data
    GURL media_url(*src);
    if (!media_url.SchemeIs(url::kHttpsScheme) && !media_url.SchemeIsBlob()) {
      continue;
    }

    if (media_url.SchemeIsBlob() &&
        !GURL(media_url.path()).SchemeIs(url::kHttpsScheme)) {
      // Double checking if the blob: is followed by https:// scheme.
      // https://github.com/brave/playlist-component/pull/39#discussion_r1445408827
      continue;
    }

    item->media_source = media_url;
    item->media_path = media_url;
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

bool PlaylistDownloadRequestManager::
    ShouldExtractMediaFromBackgroundWebContents(
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

void PlaylistDownloadRequestManager::ConfigureWebPrefsForBackgroundWebContents(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* web_prefs) {
  if (!service_->playlist_enabled()) {
    return;
  }

  web_prefs->should_detect_media_files = true;

  if (web_contents_ && web_contents_.get() == web_contents) {
    // Background web contents.
    web_prefs->force_cosmetic_filtering = true;
    web_prefs->hide_media_src_api = true;
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
