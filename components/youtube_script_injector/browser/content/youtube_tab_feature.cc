// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/content/youtube_tab_feature.h"

#include <string>
#include <utility>
#include <memory>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/browser/android/tab_features_android.h"
#include "brave/build/android/jni_headers/BackgroundVideoPlaybackTabHelper_jni.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"
#include "brave/components/youtube_script_injector/common/features.h"
#include "brave/components/youtube_script_injector/common/pref_names.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace youtube_script_injector {

// static
void YouTubeTabFeature::EnterPipMode() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BackgroundVideoPlaybackTabHelper_enterPipMode(env);
}

YouTubeTabFeature::YouTubeTabFeature(content::WebContents* web_contents,
                                     const int32_t world_id)
    : WebContentsObserver(web_contents),
      world_id_(world_id),
      youtube_registry_(YouTubeRegistry::GetInstance()) {
  DCHECK(youtube_registry_);
}

YouTubeTabFeature::~YouTubeTabFeature() = default;

void YouTubeTabFeature::InsertScriptInPage(
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
            world_id_, base::UTF8ToUTF16(script), activation,
            blink::mojom::PromiseResultOption::kDoNotWait, base::DoNothing());
  } else {
    VLOG(2) << "render_frame_host is invalid.";
    return;
  }
}

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>&
YouTubeTabFeature::GetRemote(content::RenderFrameHost* rfh) {
  if (!script_injector_remote_.is_bound()) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
  }
  return script_injector_remote_;
}

const std::optional<YouTubeJson>& YouTubeTabFeature::GetJson() const {
  return youtube_registry_->GetJson();
}

bool YouTubeTabFeature::IsBackgroundVideoPlaybackEnabled(
    content::WebContents* contents) {
  PrefService* prefs =
      user_prefs::UserPrefs::Get(contents->GetBrowserContext());

  return (
      base::FeatureList::IsEnabled(features::kBraveBackgroundVideoPlayback) &&
      prefs->GetBoolean(prefs::kYouTubeBackgroundVideoPlaybackEnabled));
}

bool YouTubeTabFeature::AreYouTubeExtraControlsEnabled(
    content::WebContents* contents) {
  PrefService* prefs =
      user_prefs::UserPrefs::Get(contents->GetBrowserContext());

  return (base::FeatureList::IsEnabled(features::kBraveYouTubeExtraControls) &&
          prefs->GetBoolean(prefs::kYouTubeExtraControlsEnabled));
}

void YouTubeTabFeature::PrimaryMainDocumentElementAvailable() {
  auto url = web_contents()->GetLastCommittedURL();
  if (!youtube_registry_->IsYouTubeDomain(url)) {
    return;
  }

  if (!IsBackgroundVideoPlaybackEnabled(web_contents()) &&
      !AreYouTubeExtraControlsEnabled(web_contents())) {
    return;
  }

  const std::optional<YouTubeJson>& json = youtube_registry_->GetJson();
  if (!json) {
    return;
  }

  content::GlobalRenderFrameHostId render_frame_host_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();

  if (AreYouTubeExtraControlsEnabled(web_contents())) {
    youtube_registry_->LoadScriptFromPath(
        url, json->GetPipScript(),
        base::BindOnce(&YouTubeTabFeature::InsertScriptInPage,
                       weak_factory_.GetWeakPtr(), render_frame_host_id,
                       blink::mojom::UserActivationOption::kDoNotActivate));
  }

  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    youtube_registry_->LoadScriptFromPath(
        url, json->GetPlaybackVideoScript(),
        base::BindOnce(&YouTubeTabFeature::InsertScriptInPage,
                       weak_factory_.GetWeakPtr(), render_frame_host_id,
                       blink::mojom::UserActivationOption::kDoNotActivate));
  }
}

void JNI_BackgroundVideoPlaybackTabHelper_SetFullscreen(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  if (!base::FeatureList::IsEnabled(
          youtube_script_injector::features::kBraveYouTubeScriptInjector)) {
    return;
  }
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);

  // TODO: replace with appropriate method to fetch TabFeature once ready.
  YouTubeTabFeature* helper =
      new YouTubeTabFeature(web_contents, ISOLATED_WORLD_ID_BRAVE_INTERNAL);

  if (!helper || !helper->GetJson()) {
    return;
  }

  if (!helper->AreYouTubeExtraControlsEnabled(web_contents)) {
    return;
  }

  auto* registry = YouTubeRegistry::GetInstance();
  if (!registry) {
    return;
  }

  auto url = web_contents->GetLastCommittedURL();
  registry->LoadScriptFromPath(
      url, helper->GetJson()->GetFullscreenScript(),
      base::BindOnce(&YouTubeTabFeature::InsertScriptInPage,
                     // TODO: replace with `helper->GetWeakPtr(),`
                     base::Owned(helper),
                     web_contents->GetPrimaryMainFrame()->GetGlobalId(),
                     blink::mojom::UserActivationOption::kActivate));
}

}  // namespace youtube_script_injector
