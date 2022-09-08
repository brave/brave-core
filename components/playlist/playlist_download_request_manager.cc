/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_download_request_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
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

// TODO(simonhong): Move this to separated file.
class PageSpecificContentSettingsDelegate
    : public content_settings::PageSpecificContentSettings::Delegate {
 public:
  PageSpecificContentSettingsDelegate(PrefService* prefs,
                                      HostContentSettingsMap* map)
      : prefs_(prefs), map_(map) {}
  ~PageSpecificContentSettingsDelegate() override = default;
  PageSpecificContentSettingsDelegate(
      const PageSpecificContentSettingsDelegate&) = delete;
  PageSpecificContentSettingsDelegate& operator=(
      const PageSpecificContentSettingsDelegate&) = delete;

 private:
  // PageSpecificContentSettings::Delegate:
  void UpdateLocationBar() override {}
  PrefService* GetPrefs() override { return prefs_; }
  HostContentSettingsMap* GetSettingsMap() override { return map_; }
  void SetDefaultRendererContentSettingRules(
      content::RenderFrameHost* rfh,
      RendererContentSettingRules* rules) override {}
  std::vector<storage::FileSystemType> GetAdditionalFileSystemTypes() override {
    return {};
  }
  browsing_data::CookieHelper::IsDeletionDisabledCallback
  GetIsDeletionDisabledCallback() override {
    return base::NullCallback();
  }
  bool IsMicrophoneCameraStateChanged(
      content_settings::PageSpecificContentSettings::MicrophoneCameraState
          microphone_camera_state,
      const std::string& media_stream_selected_audio_device,
      const std::string& media_stream_selected_video_device) override {
    return false;
  }
  content_settings::PageSpecificContentSettings::MicrophoneCameraState
  GetMicrophoneCameraState() override {
    return content_settings::PageSpecificContentSettings::
        MICROPHONE_CAMERA_NOT_ACCESSED;
  }
  void OnContentAllowed(ContentSettingsType type) override {}
  void OnContentBlocked(ContentSettingsType type) override {}
  void OnStorageAccessAllowed(
      content_settings::mojom::ContentSettingsManager::StorageType storage_type,
      const url::Origin& origin,
      content::Page& page) override {}
  void OnCookieAccessAllowed(const net::CookieList& accessed_cookies,
                             content::Page& page) override {}
  void OnServiceWorkerAccessAllowed(const url::Origin& origin,
                                    content::Page& page) override {}

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<HostContentSettingsMap> map_ = nullptr;
};

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
    HostContentSettingsMap* map,
    MediaDetectorComponentManager* manager)
    : context_(context), map_(map), media_detector_component_manager_(manager) {
  observed_.Observe(media_detector_component_manager_);

  media_detector_script_ = media_detector_component_manager_->script();
}

PlaylistDownloadRequestManager::~PlaylistDownloadRequestManager() = default;

void PlaylistDownloadRequestManager::CreateWebContents() {
  if (!web_contents_) {
    // |web_contents_| is created on demand.
    content::WebContents::CreateParams create_params(context_, nullptr);
    web_contents_ = content::WebContents::Create(create_params);
    content_settings::PageSpecificContentSettings::CreateForWebContents(
        web_contents_.get(),
        std::make_unique<PageSpecificContentSettingsDelegate>(
            user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext()),
            map_));
  }

  Observe(web_contents_.get());
}

void PlaylistDownloadRequestManager::GetMediaFilesFromPage(Request request) {
  web_contents_destroy_timer_.reset();

  if (!ReadyToRunMediaDetectorScript()) {
    pending_requests_.push_back(std::move(request));
    if (media_detector_script_.empty())
      media_detector_component_manager_->RegisterIfNeeded();
    return;
  }

  RunMediaDetector(std::move(request));
}

void PlaylistDownloadRequestManager::OnScriptReady(const std::string& script) {
  media_detector_script_ = script;

  FetchPendingRequest();
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
  return !media_detector_script_.empty() && in_progress_urls_count_ == 0;
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
  DCHECK(contents && contents->GetPrimaryMainFrame());

  contents->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::UTF8ToUTF16(media_detector_script_),
      base::BindOnce(&PlaylistDownloadRequestManager::OnGetMedia,
                     weak_factory_.GetWeakPtr(), contents->GetWeakPtr()),
      g_playlist_javascript_world_id);
}

void PlaylistDownloadRequestManager::OnGetMedia(
    base::WeakPtr<content::WebContents> contents,
    base::Value value) {
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
    VLOG(2) << "No media was detected";
    return;
  }

  if (!value.is_list()) {
    LOG(ERROR) << __func__
               << " Got invalid value after running media detector script";
    return;
  }

  std::vector<PlaylistItemInfo> items;
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

    PlaylistItemInfo info;
    info.id = base::Token::CreateRandom().ToString();
    info.page_src = *page_source;
    info.title = *name;
    if (thumbnail) {
      info.thumbnail_src = *thumbnail;
      info.thumbnail_path = *thumbnail;
    }
    info.media_src = *src;
    info.media_file_path = *src;
    items.push_back(std::move(info));
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

}  // namespace playlist
