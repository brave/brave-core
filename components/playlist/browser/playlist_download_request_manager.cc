/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_download_request_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/isolated_world_ids.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/page_transition_types.h"

namespace playlist {

namespace {

constexpr base::TimeDelta kWebContentDestroyDelay = base::Minutes(5);

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
  DCHECK(id > content::ISOLATED_WORLD_ID_CONTENT_END);
  // Only allow ID to be set once.
  DCHECK(!PlaylistJavaScriptWorldIdIsSet());
  g_playlist_javascript_world_id = id;
}

PlaylistDownloadRequestManager::PlaylistDownloadRequestManager(
    content::BrowserContext* context,
    MediaDetectorComponentManager* manager)
    : context_(context), media_detector_component_manager_(manager) {}

PlaylistDownloadRequestManager::~PlaylistDownloadRequestManager() = default;

void PlaylistDownloadRequestManager::CreateWebContents() {
  if (!web_contents_) {
    // |web_contents_| is created on demand.
    content::WebContents::CreateParams create_params(context_, nullptr);
    web_contents_ = content::WebContents::Create(create_params);
  }

  Observe(web_contents_.get());
}

void PlaylistDownloadRequestManager::GetMediaFilesFromPage(Request request) {
  web_contents_destroy_timer_.reset();

  if (!ReadyToRunMediaDetectorScript()) {
    pending_requests_.push_back(std::move(request));
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
  DCHECK(PlaylistJavaScriptWorldIdIsSet());

  DCHECK_GE(in_progress_urls_count_, 0);
  in_progress_urls_count_++;

  DCHECK(callback_for_current_request_.is_null());
  callback_for_current_request_ = std::move(request.callback);

  if (absl::holds_alternative<std::string>(request.url_or_contents)) {
    CreateWebContents();
    GURL url(absl::get<std::string>(request.url_or_contents));
    DCHECK(url.is_valid());
    DCHECK(web_contents_);
    web_contents_->GetController().LoadURLWithParams(
        content::NavigationController::LoadURLParams(url));
  } else {
    auto weak_contents =
        absl::get<base::WeakPtr<content::WebContents>>(request.url_or_contents);
    if (!weak_contents) {
      FetchPendingRequest();
      return;
    }

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

  if (in_progress_urls_count_ == 0 || callback_for_current_request_.is_null())
    return;

  GetMedia(web_contents_.get());
}

void PlaylistDownloadRequestManager::GetMedia(content::WebContents* contents) {
  DVLOG(2) << __func__;
  DCHECK(contents && contents->GetPrimaryMainFrame());

  const auto& media_detector_script =
      media_detector_component_manager_->GetMediaDetectorScript();
  DCHECK(!media_detector_script.empty());

  contents->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(media_detector_script),
      base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                     weak_factory_.GetWeakPtr(), contents->GetWeakPtr()),
      g_playlist_javascript_world_id);
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
  if (!contents)
    return;

  DCHECK(!callback_for_current_request_.is_null()) << " callback already ran";
  auto callback = std::move(callback_for_current_request_);

  DCHECK_GT(in_progress_urls_count_, 0);
  in_progress_urls_count_--;

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

  if (in_progress_urls_count_ == 0)
    ScheduleWebContentsDestroying();
  Observe(nullptr);

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

    auto* name = media.FindStringKey("name");
    auto* page_title = media.FindStringKey("pageTitle");
    auto* page_source = media.FindStringKey("pageSrc");
    auto* mime_type = media.FindStringKey("mimeType");
    auto* src = media.FindStringKey("src");
    auto* thumbnail = media.FindStringKey("thumbnail");
    DCHECK(name);
    DCHECK(page_source);
    DCHECK(page_title);
    DCHECK(mime_type);
    DCHECK(src);

    auto item = mojom::PlaylistItem::New();
    item->id = base::Token::CreateRandom().ToString();
    item->page_source = GURL(*page_source);
    item->name = *name;
    if (thumbnail) {
      item->thumbnail_source = GURL(*thumbnail);
      item->thumbnail_path = GURL(*thumbnail);
    }
    item->media_source = GURL(*src);
    item->media_path = GURL(*src);
    items.push_back(std::move(item));
  }

  std::move(callback).Run(std::move(items));
}

void PlaylistDownloadRequestManager::ScheduleWebContentsDestroying() {
  if (!web_contents_destroy_timer_) {
    web_contents_destroy_timer_ = std::make_unique<base::RetainingOneShotTimer>(
        FROM_HERE, kWebContentDestroyDelay,
        base::BindRepeating(&PlaylistDownloadRequestManager::DestroyWebContents,
                            base::Unretained(this)));
  }
  web_contents_destroy_timer_->Reset();
}

void PlaylistDownloadRequestManager::DestroyWebContents() {
  web_contents_.reset();
}

void PlaylistDownloadRequestManager::ConfigureWebPrefsForBackgroundWebContents(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* web_prefs) {
  if (web_contents_ && web_contents_.get() == web_contents) {
    web_prefs->force_cosmetic_filtering = true;
    web_prefs->hide_media_src_api = true;
  }
}

content::WebContents*
PlaylistDownloadRequestManager::GetBackgroundWebContentsForTesting() {
  CreateWebContents();
  return web_contents_.get();
}

}  // namespace playlist
