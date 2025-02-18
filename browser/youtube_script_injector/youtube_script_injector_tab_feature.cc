// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/youtube_script_injector/youtube_script_injector_tab_feature.h"

#include "brave/build/android/jni_headers/YouTubeScriptInjectorTabFeature_jni.h"
#include "brave/components/youtube_script_injector/browser/content/youtube_tab_feature.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"
#include "brave/components/youtube_script_injector/common/features.h"
#include "chrome/common/chrome_isolated_world_ids.h"

namespace youtube_script_injector {

void YouTubeScriptInjectorTabFeature::EnterPipMode() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_YouTubeScriptInjectorTabFeature_enterPipMode(env);
}

void JNI_YouTubeScriptInjectorTabFeature_SetFullscreen(
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
                     // TODO: replace with `helper->GetWeakPtr()`
                     base::Owned(helper), web_contents->GetPrimaryMainFrame(),
                     blink::mojom::UserActivationOption::kActivate));
}

}  // namespace youtube_script_injector
