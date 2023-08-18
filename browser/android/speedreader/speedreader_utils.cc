/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/build/android/jni_headers/BraveSpeedReaderUtils_jni.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/web_contents.h"

namespace speedreader {

static void JNI_BraveSpeedReaderUtils_ToggleEnabledForWebContent(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents,
    jboolean enabled) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);

  auto* speedreader = SpeedreaderServiceFactory::GetForBrowserContext(
      web_contents->GetBrowserContext());
  if (!speedreader) {
    LOG(ERROR) << "no speedreader service";
    return;
  }
  speedreader->EnableForSite(web_contents, enabled);
}

static jboolean JNI_BraveSpeedReaderUtils_TabProbablyReadable(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& tab) {
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab);
  content::WebContents* web_contents = tab_android->web_contents();
  if (!web_contents) {
    return false;
  }

  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return false;
  }

  auto* speedreader = SpeedreaderServiceFactory::GetForBrowserContext(
      web_contents->GetBrowserContext());
  if (!speedreader) {
    return false;
  }

  return DistillStates::IsDistillable(tab_helper->PageDistillState()) &&
         !speedreader->IsEnabledForSite(web_contents);
}

static jboolean JNI_BraveSpeedReaderUtils_TabStateIsDistilled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& tab) {
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab);
  content::WebContents* web_contents = tab_android->web_contents();
  if (!web_contents) {
    return false;
  }

  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return false;
  }

  return DistillStates::IsDistilled(tab_helper->PageDistillState());
}

static jboolean JNI_BraveSpeedReaderUtils_TabSupportsDistillation(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& tab) {
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab);
  content::WebContents* web_contents = tab_android->web_contents();
  if (!web_contents) {
    return false;
  }

  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return false;
  }

  auto* speedreader = SpeedreaderServiceFactory::GetForBrowserContext(
      web_contents->GetBrowserContext());
  if (!speedreader) {
    return false;
  }

  return DistillStates::IsDistillable(tab_helper->PageDistillState()) ||
         speedreader->IsEnabledForSite(web_contents);
}

static void JNI_BraveSpeedReaderUtils_SingleShotSpeedreaderForWebContent(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    LOG(ERROR) << "speedreader_tab_helper: no tab_helper!";
    return;
  }
  if (DistillStates::IsViewOriginal(tab_helper->PageDistillState())) {
    tab_helper->ProcessIconClick();
  }
}
}  // namespace speedreader
