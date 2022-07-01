/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_download_request_manager.h"

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/isolated_world_ids.h"
#include "third_party/re2/src/re2/re2.h"

namespace playlist {

namespace {

constexpr base::TimeDelta kWebContentDestroyDelay = base::Minutes(5);

const int32_t kInvalidWorldID = -1;

int32_t g_playlist_javascript_world_id = kInvalidWorldID;

bool PlaylistJavaScriptWorldIdIsSet() {
  return g_playlist_javascript_world_id != kInvalidWorldID;
}

}  // namespace

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
    Delegate* delegate,
    MediaDetectorComponentManager* manager)
    : context_(context),
      delegate_(delegate),
      media_detector_component_manager_(manager) {
  observed_.Observe(media_detector_component_manager_);
  media_detector_script_ = media_detector_component_manager_->script();
}

PlaylistDownloadRequestManager::~PlaylistDownloadRequestManager() = default;

void PlaylistDownloadRequestManager::CreateWebContents() {
  if (web_contents_)
    return;

  // |web_contents_| is created on demand.
  constexpr char kYoutubeURL[] = "https://www.youtube.com/";
  content::WebContents::CreateParams create_params(context_, nullptr);
  web_contents_ = content::WebContents::Create(create_params);
  web_contents_->GetController().LoadURLWithParams(
      content::NavigationController::LoadURLParams(GURL(kYoutubeURL)));
  Observe(web_contents_.get());
}

void PlaylistDownloadRequestManager::GeneratePlaylistCreateParamsForYoutubeURL(
    const std::string& url) {
  web_contents_destroy_timer_.reset();

  if (!ReadyToRunMediaDetectorScript()) {
    pending_youtube_urls_.push_back(url);

    CreateWebContents();
    media_detector_component_manager_->RegisterIfNeeded();
    return;
  }

  RunMediaDetector(url);
}

void PlaylistDownloadRequestManager::OnScriptReady(const std::string& script) {
  media_detector_script_ = script;

  if (ReadyToRunMediaDetectorScript())
    FetchAllPendingYoutubeURLs();
}

void PlaylistDownloadRequestManager::FetchAllPendingYoutubeURLs() {
  if (pending_youtube_urls_.empty())
    return;

  // Run all pending request.
  for (const auto& url : pending_youtube_urls_) {
    RunMediaDetector(url);
  }

  pending_youtube_urls_.clear();
}

void PlaylistDownloadRequestManager::RunMediaDetector(const std::string& url) {
  DCHECK(PlaylistJavaScriptWorldIdIsSet());

  DCHECK_GE(in_progress_youtube_urls_count_, 0);
  in_progress_youtube_urls_count_++;

  web_contents_->GetMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(media_detector_script_),
      base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                     weak_factory_.GetWeakPtr()),
      g_playlist_javascript_world_id);
}

bool PlaylistDownloadRequestManager::ReadyToRunMediaDetectorScript() const {
  if (!media_detector_script_.empty() && web_contents_ && web_contents_ready_)
    return true;

  return false;
}

void PlaylistDownloadRequestManager::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (render_frame_host->GetMainFrame()) {
    // To run youtubedown.js, frame should be ready.
    // After that, we don't need to observe.
    web_contents_ready_ = true;
    Observe(nullptr);

    if (ReadyToRunMediaDetectorScript())
      FetchAllPendingYoutubeURLs();
  }
}

void PlaylistDownloadRequestManager::OnGetMedia(base::Value value) {
  DCHECK_GT(in_progress_youtube_urls_count_, 0);
  in_progress_youtube_urls_count_--;

  if (in_progress_youtube_urls_count_ == 0)
    ScheduleWebContentsDestroying();

  if (!value.is_list()) {
    LOG(ERROR) << __func__
               << " Got invalid value after running media detector script";
    return;
  }

  CreatePlaylistParams params;

  const auto& list = value.GetList();
  if (list.empty()) {
    LOG(ERROR) << __func__
               << " Got invalid value after running media detector script";
    return;
  }

  const bool has_audio = list.size() > 1;

  // Get clean playlist name.
  // TODO(sko) This routine is set for youtubedown.js. We need to clean this up
  // with new script.
  auto* file = list[0].FindStringKey("file");
  auto* thumb = list[0].FindStringKey("thumb");
  DCHECK(file);
  DCHECK(thumb);

  params.playlist_name = *file;
  RE2::Replace(&params.playlist_name, "\\s\\[.*$", "");

  params.playlist_thumbnail_url = *thumb;
  if (const std::string* url = list[0].FindStringKey("url")) {
    params.video_media_files.emplace_back(*url, "");
  } else {
    DCHECK(list[0].FindListKey("url"));
    for (const auto& value : list[0].FindListKey("url")->GetList()) {
      params.video_media_files.emplace_back(value.GetString(), "");
    }
  }

  if (has_audio) {
    if (const std::string* url = list[1].FindStringKey("url")) {
      params.audio_media_files.emplace_back(*url, "");
    } else {
      DCHECK(list[0].FindListKey("url"));
      for (const auto& value : list[1].FindListKey("url")->GetList()) {
        params.audio_media_files.emplace_back(value.GetString(), "");
      }
    }
  }

  delegate_->OnPlaylistCreationParamsReady(params);
}

void PlaylistDownloadRequestManager::ScheduleWebContentsDestroying() {
  DCHECK(!web_contents_destroy_timer_);
  web_contents_destroy_timer_ = std::make_unique<base::OneShotTimer>();
  web_contents_destroy_timer_->Start(
      FROM_HERE, kWebContentDestroyDelay,
      base::BindOnce(&PlaylistDownloadRequestManager::DestroyWebContents,
                     base::Unretained(this)));
}

void PlaylistDownloadRequestManager::DestroyWebContents() {
  web_contents_.reset();
}

}  // namespace playlist
