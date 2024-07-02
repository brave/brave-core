/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/day_zero_browser_ui_expt/android/day_zero_browser_ui_expt.h"

#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "brave/build/android/jni_headers/DayZeroMojomHelper_jni.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/android/browser_context_handle.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace day_zero {

DayZeroBrowserUiExpt::DayZeroBrowserUiExpt(content::BrowserContext* context) {
  // auto skus_service_getter = base::BindRepeating(
  //     [](content::BrowserContext* context) {
  //       return skus::SkusServiceFactory::GetForContext(context);
  //     },
  //     context);
  // pref_service_ = Profile::FromBrowserContext(context)->GetPrefs();
  // credential_manager_ = std::make_unique<ai_chat::AIChatCredentialManager>(
  //     skus_service_getter, g_browser_process->local_state());
}

DayZeroBrowserUiExpt::~DayZeroBrowserUiExpt() = default;

void DayZeroBrowserUiExpt::BindInterface(
    mojo::PendingReceiver<mojom::DayZeroBrowserUiExpt> pending_receiver) {
  receivers_.Add(this, std::move(pending_receiver));
}

static jlong JNI_DayZeroMojomHelper_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  content::BrowserContext* browser_context =
      content::BrowserContextFromJavaHandle(jbrowser_context_handle);
  DayZeroBrowserUiExpt* settings_helper =
      new DayZeroBrowserUiExpt(browser_context);
  return reinterpret_cast<intptr_t>(settings_helper);
}

void DayZeroBrowserUiExpt::Destroy(JNIEnv* env) {
  delete this;
}

jlong DayZeroBrowserUiExpt::GetInterfaceToAndroidHelper(JNIEnv* env) {
  mojo::PendingRemote<mojom::DayZeroBrowserUiExpt> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());

  return static_cast<jlong>(remote.PassPipe().release().value());
}

void DayZeroBrowserUiExpt::IsDayZeroExpt(IsDayZeroExptCallback callback) {
  std::move(callback).Run(true);
}

}  // namespace day_zero
