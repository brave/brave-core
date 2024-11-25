// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/content/youtube_tab_helper.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/build/android/jni_headers/BackgroundVideoPlaybackTabHelper_jni.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"
#include "brave/components/youtube_script_injector/common/features.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace youtube_script_injector {

// static
void YouTubeTabHelper::MaybeCreateForWebContents(content::WebContents* contents,
                                              const int32_t world_id) {
  if (contents->GetBrowserContext()->IsOffTheRecord() ||
      !base::FeatureList::IsEnabled(youtube_script_injector::features::kBraveYouTubeScriptInjector)) {
    return;
  }

  youtube_script_injector::YouTubeTabHelper::CreateForWebContents(contents, world_id);
}

YouTubeTabHelper::YouTubeTabHelper(content::WebContents* web_contents,
                             const int32_t world_id)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<YouTubeTabHelper>(*web_contents),
      world_id_(world_id),
      youtube_registry_(YouTubeRegistry::GetInstance()) {
  DCHECK(youtube_registry_);
}

YouTubeTabHelper::~YouTubeTabHelper() = default;

void YouTubeTabHelper::InsertScriptInPage(
    const content::GlobalRenderFrameHostId& render_frame_host_id,
    blink::mojom::UserActivationOption activation,
    std::string script) {
// Early return if script is empty.
  if (script.empty()) {
    VLOG(2) << "Script is empty, skipping injection.";
    return;
  }
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_frame_host_id);

  // Check if render_frame_host is still valid and if starting rfh is the same.
  if (render_frame_host &&
      render_frame_host_id ==
          web_contents()->GetPrimaryMainFrame()->GetGlobalId()) {
    GetRemote(render_frame_host)
        ->RequestAsyncExecuteScript(
            world_id_, base::UTF8ToUTF16(script),
            activation,
            blink::mojom::PromiseResultOption::kDoNotWait, base::DoNothing());
  } else {
    VLOG(2) << "render_frame_host is invalid.";
    return;
  }
}

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
YouTubeTabHelper::GetRemote(content::RenderFrameHost* rfh) {
  if (!script_injector_remote_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
  }
  return script_injector_remote_;
}

const std::optional<YouTubeJson>& YouTubeTabHelper::GetJson() const { return youtube_registry_->GetJson(); }

void YouTubeTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {

  const std::optional<YouTubeJson>& json = youtube_registry_->GetJson();
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument() ||
      !json) {
    return;
  }

  auto url = web_contents()->GetLastCommittedURL();
  if (!youtube_registry_->IsYouTubeDomain(url)) {
    return;
  }

  content::GlobalRenderFrameHostId render_frame_host_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();

  youtube_registry_->LoadScriptFromPath(
      url, json->GetFeatureScript(), base::BindOnce(&YouTubeTabHelper::InsertScriptInPage,
                          weak_factory_.GetWeakPtr(), render_frame_host_id, blink::mojom::UserActivationOption::kDoNotActivate));
}

void YouTubeTabHelper::MediaStartedPlaying(
    const MediaPlayerInfo& /*video_type*/,
    const content::MediaPlayerId& id) {
  is_media_playing_ = true;
}

void YouTubeTabHelper::MediaStoppedPlaying(
    const MediaPlayerInfo& /*video_type*/,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason /*reason*/) {
  is_media_playing_ = false;
}

// bool IsBackgroundVideoPlaybackEnabled(content::WebContents* contents) {
//   PrefService* prefs =
//       static_cast<Profile*>(contents->GetBrowserContext())->GetPrefs();

//   if (!base::FeatureList::IsEnabled(
//           ::preferences::features::kBraveBackgroundVideoPlayback) &&
//       !prefs->GetBoolean(kBackgroundVideoPlaybackEnabled)) {
//     return false;
//   }

//   return true;
// }

WEB_CONTENTS_USER_DATA_KEY_IMPL(YouTubeTabHelper);
}  // namespace youtube_script_injector

namespace chrome {
namespace android {

void JNI_BackgroundVideoPlaybackTabHelper_SetFullscreen(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);

  // Get the YouTubeTabHelper instance
  youtube_script_injector::YouTubeTabHelper* helper = youtube_script_injector::YouTubeTabHelper::FromWebContents(web_contents);
  if (!helper || !helper->GetJson()) {
    return;
  }

  auto* registry = youtube_script_injector::YouTubeRegistry::GetInstance();
  if (!registry) {
    return;
  }

  auto url = web_contents->GetLastCommittedURL();
  registry->LoadScriptFromPath(
      url,
      helper->GetJson()->GetFullscreenScript(),
      base::BindOnce(&youtube_script_injector::YouTubeTabHelper::InsertScriptInPage,
                          helper->GetWeakPtr(),
                          web_contents->GetPrimaryMainFrame()->GetGlobalId(),
                          blink::mojom::UserActivationOption::kActivate));
}

jboolean JNI_BackgroundVideoPlaybackTabHelper_IsPlayingMedia(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
      content::WebContents* web_contents =
          content::WebContents::FromJavaWebContents(jweb_contents);
      auto* helper = youtube_script_injector::YouTubeTabHelper::FromWebContents(web_contents);
      return helper ? helper->IsMediaPlaying() : false;
}
}  // namespace android
}  // namespace chrome
