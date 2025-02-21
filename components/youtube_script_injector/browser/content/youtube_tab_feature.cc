// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/content/youtube_tab_feature.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
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

YouTubeTabFeature::YouTubeTabFeature(content::WebContents* web_contents,
                                     const int32_t world_id)
    : WebContentsObserver(web_contents),
      world_id_(world_id),
      youtube_registry_(YouTubeRegistry::GetInstance()) {
  DCHECK(youtube_registry_);
}

YouTubeTabFeature::~YouTubeTabFeature() = default;

void YouTubeTabFeature::InsertScriptInPage(
    content::RenderFrameHost* render_frame_host,
    blink::mojom::UserActivationOption activation,
    std::string script) {
  // Early return if script is empty.
  if (script.empty()) {
    VLOG(2) << "Script is empty, skipping injection.";
    return;
  }

  if (render_frame_host) {
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

  if (AreYouTubeExtraControlsEnabled(web_contents())) {
    youtube_registry_->LoadScriptFromPath(
        url, json->GetScript(YouTubeJson::ScriptType::PIP),
        base::BindOnce(&YouTubeTabFeature::InsertScriptInPage,
                       weak_factory_.GetWeakPtr(),
                       web_contents()->GetPrimaryMainFrame(),
                       blink::mojom::UserActivationOption::kDoNotActivate));
  }

  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    youtube_registry_->LoadScriptFromPath(
        url, json->GetScript(YouTubeJson::ScriptType::PLAYBACK_VIDEO),
        base::BindOnce(&YouTubeTabFeature::InsertScriptInPage,
                       weak_factory_.GetWeakPtr(),
                       web_contents()->GetPrimaryMainFrame(),
                       blink::mojom::UserActivationOption::kDoNotActivate));
  }
}

}  // namespace youtube_script_injector
