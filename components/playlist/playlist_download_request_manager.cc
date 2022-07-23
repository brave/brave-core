/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_download_request_manager.h"

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
  content::WebContents::CreateParams create_params(context_, nullptr);
  web_contents_ = content::WebContents::Create(create_params);

  Observe(web_contents_.get());
}

void PlaylistDownloadRequestManager::GetMediaFilesFromPage(
    const std::string& url) {
  web_contents_destroy_timer_.reset();

  CreateWebContents();
  if (!ReadyToRunMediaDetectorScript()) {
    pending_urls_.push_back(url);
    media_detector_component_manager_->RegisterIfNeeded();
    return;
  }

  RunMediaDetector(url);
}

void PlaylistDownloadRequestManager::OnScriptReady(const std::string& script) {
  media_detector_script_ = script;

  if (ReadyToRunMediaDetectorScript())
    FetchPendingURL();
}

void PlaylistDownloadRequestManager::FetchPendingURL() {
  if (pending_urls_.empty())
    return;

  auto url = pending_urls_.front();
  pending_urls_.pop_front();
  RunMediaDetector(url);
}

void PlaylistDownloadRequestManager::RunMediaDetector(const std::string& url) {
  DCHECK(PlaylistJavaScriptWorldIdIsSet());

  DCHECK_GE(in_progress_urls_count_, 0);
  in_progress_urls_count_++;

  web_contents_->GetController().LoadURLWithParams(
      content::NavigationController::LoadURLParams(GURL(url)));
}

bool PlaylistDownloadRequestManager::ReadyToRunMediaDetectorScript() const {
  return !media_detector_script_.empty();
}

void PlaylistDownloadRequestManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK(web_contents_->GetMainFrame());

  // This script is from
  // https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/PlaylistSwizzler.js
  static const std::u16string kScriptToHideMediaSourceAPI =
      uR"-(
    (function() {
      // Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
      if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
        window.MediaSource = null;
        window.WebKitMediaSource = null;
        delete window.MediaSource;
        delete window.WebKitMediaSource;
      }
    })();
    )-";

  // In order to hide js API from main world, use testing
  // api temporarily.
  web_contents_->GetMainFrame()->ExecuteJavaScriptForTests(
      kScriptToHideMediaSourceAPI, base::NullCallback());
}

void PlaylistDownloadRequestManager::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (in_progress_urls_count_ == 0)
    return;

  DCHECK(web_contents_->GetMainFrame());

  web_contents_->GetMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(media_detector_script_),
      base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                     weak_factory_.GetWeakPtr()),
      g_playlist_javascript_world_id);
}

void PlaylistDownloadRequestManager::OnGetMedia(base::Value value) {
  auto print_value = [](const base::Value& value) {
    std::string out;
    base::JSONWriter::Write(value, &out);
    LOG(ERROR) << value;
  };

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

  if (!value.is_list()) {
    LOG(ERROR) << __func__
               << " Got invalid value after running media detector script:";
    print_value(value);
    return;
  }

  for (const auto& media : value.GetList()) {
    if (!media.is_dict()) {
      LOG(ERROR) << __func__
                 << " Got invalid value after running media detector script";
      print_value(media);
      return;
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

    PlaylistItemInfo info;
    info.title = *name;
    if (thumbnail)
      info.thumbnail_path = *thumbnail;
    info.media_file_path = *src;
    delegate_->OnPlaylistCreationParamsReady(info);
  }

  FetchPendingURL();
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
