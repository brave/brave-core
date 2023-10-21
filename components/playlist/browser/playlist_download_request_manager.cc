/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_download_request_manager.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
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

const int32_t kInvalidWorldID = -1;

int32_t g_playlist_javascript_world_id = kInvalidWorldID;

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

PlaylistDownloadRequestManager::PlaylistDownloadRequestManager(
    content::BrowserContext* context,
    MediaDetectorComponentManager* manager)
    : context_(context), media_detector_component_manager_(manager) {}

PlaylistDownloadRequestManager::~PlaylistDownloadRequestManager() = default;

void PlaylistDownloadRequestManager::CreateWebContents(
    bool should_force_fake_ua) {
  content::WebContents::CreateParams create_params(context_, nullptr);
  web_contents_ = content::WebContents::Create(create_params);
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

  Observe(web_contents_.get());
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
  CHECK(PlaylistJavaScriptWorldIdIsSet());

  DCHECK_GE(in_progress_urls_count_, 0);
  in_progress_urls_count_++;

  DCHECK(callback_for_current_request_.is_null());
  DCHECK(requested_url_.is_empty());

  callback_for_current_request_ = std::move(request.callback);
  DCHECK(callback_for_current_request_)
      << "Empty callback shouldn't be requested";
  request_start_time_ = base::Time::Now();

  if (absl::holds_alternative<std::string>(request.url_or_contents)) {
    // Start to request on clean slate, so that result won't be affected by
    // previous page.
    CreateWebContents(request.should_force_fake_ua);

    requested_url_ = GURL(absl::get<std::string>(request.url_or_contents));
    DCHECK(requested_url_.is_valid());
    DCHECK(web_contents_);
    DVLOG(2) << "Load URL to detect media files: " << requested_url_.spec();
    auto load_url_params =
        content::NavigationController::LoadURLParams(requested_url_);
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
  } else {
    auto weak_contents =
        absl::get<base::WeakPtr<content::WebContents>>(request.url_or_contents);
    if (!weak_contents) {
      // While the request was in queue, the tab was deleted. Proceed to the
      // next request.
      in_progress_urls_count_--;
      auto callback = std::move(callback_for_current_request_);
      DCHECK(callback) << "Callback should be valid but we won't run this";

      FetchPendingRequest();
      return;
    }

    DVLOG(2) << "Try detecting media files from existing web contents: "
             << web_contents_->GetVisibleURL();
    requested_url_ = weak_contents->GetVisibleURL();
    GetMedia(weak_contents.get());
  }
}

bool PlaylistDownloadRequestManager::ReadyToRunMediaDetectorScript() const {
  return in_progress_urls_count_ == 0;
}

void PlaylistDownloadRequestManager::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (render_frame_host != web_contents_->GetPrimaryMainFrame())
    return;

  if (in_progress_urls_count_ == 0 || callback_for_current_request_.is_null()) {
    // As we don't support canceling at this moment, this shouldn't happen.
    CHECK_IS_TEST();
    return;
  }

  DVLOG(2) << __func__;
  GetMedia(web_contents_.get());
}

void PlaylistDownloadRequestManager::GetMedia(content::WebContents* contents) {
  DVLOG(2) << __func__;
  DCHECK(contents && contents->GetPrimaryMainFrame());

  const auto& media_detector_script =
      media_detector_component_manager_->GetMediaDetectorScript(
          contents->GetVisibleURL());
  DCHECK(!media_detector_script.empty());

#if BUILDFLAG(IS_ANDROID)
  content::RenderFrameHost::AllowInjectingJavaScript();
  contents->GetPrimaryMainFrame()->ExecuteJavaScript(
      base::UTF8ToUTF16(media_detector_script),
      base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                     weak_factory_.GetWeakPtr(), contents->GetWeakPtr()));
#else
  if (run_script_on_main_world_) {
    contents->GetPrimaryMainFrame()->ExecuteJavaScriptForTests(
        base::UTF8ToUTF16(media_detector_script),
        base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                       weak_factory_.GetWeakPtr(), contents->GetWeakPtr()));

  } else {
    contents->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
        base::UTF8ToUTF16(media_detector_script),
        base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                       weak_factory_.GetWeakPtr(), contents->GetWeakPtr()),
        g_playlist_javascript_world_id);
  }
#endif
}

void PlaylistDownloadRequestManager::OnGetMedia(
    base::WeakPtr<content::WebContents> contents,
    base::Value value) {
  DVLOG(2) << __func__;
  ProcessFoundMedia(contents, std::move(value));
  FetchPendingRequest();
}

void PlaylistDownloadRequestManager::ProcessFoundMedia(
    base::WeakPtr<content::WebContents> contents,
    base::Value value) {
  DCHECK(!callback_for_current_request_.is_null()) << " callback already ran";
  auto callback = std::move(callback_for_current_request_);

  DCHECK_GT(in_progress_urls_count_, 0);
  in_progress_urls_count_--;
  GURL requested_url = std::move(requested_url_);
  Observe(nullptr);
  if (!contents) {
    return;
  }

  /* Expected output:
    [
      {
        "detected": boolean,
        "mimeType": "video" | "audio",
        "name": string,
        "pageSrc": url,
        "pageTitle": string
        "src": url
        "thumbnail": url | undefined
      }
    ]
  */

  if (value.is_dict() && value.GetDict().empty()) {
    DVLOG(2) << "No media was detected";
    return;
  }

  if (!value.is_list()) {
    LOG(ERROR) << __func__
               << " Got invalid value after running media detector script";
    return;
  }

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
    if (!name || !page_source || !page_title || !mime_type || !src) {
      LOG(ERROR) << __func__ << " required fields are not satisfied";
      continue;
    }

    // nullable data
    auto* thumbnail = media_dict.FindString("thumbnail");
    auto* author = media_dict.FindString("author");
    auto duration = media_dict.FindDouble("duration");

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->page_source = requested_url;
    item->page_redirected = GURL(*page_source);
    item->name = *name;
    // URL data
    if (GURL media_url(*src); !media_url.SchemeIs(url::kHttpsScheme)) {
      LOG(ERROR) << __func__ << "media scheme is not https://";
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
    item->media_source = GURL(*src);
    item->media_path = GURL(*src);
    if (author)
      item->author = *author;

    items.push_back(std::move(item));
  }

  DVLOG(2) << __func__
           << " Media detection result size: " << value.GetList().size() << " "
           << items.size();
  std::move(callback).Run(std::move(items));
}

void PlaylistDownloadRequestManager::SetRunScriptOnMainWorldForTest() {
  CHECK_IS_TEST();
  run_script_on_main_world_ = true;
}

void PlaylistDownloadRequestManager::ConfigureWebPrefsForBackgroundWebContents(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* web_prefs) {
  if (web_contents_ && web_contents_.get() == web_contents) {
    web_prefs->force_cosmetic_filtering = true;
    web_prefs->hide_media_src_api = true;
    web_prefs->should_detect_media_files = true;
  }
}

void PlaylistDownloadRequestManager::ResetRequests() {
  if (web_contents_) {
    Observe(nullptr);
    web_contents_.reset();
  }

  pending_requests_.clear();
  requested_url_ = {};
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
