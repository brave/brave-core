/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/day_zero_browser_ui_expt/android/day_zero_browser_ui_expt.h"

#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/brave_stats/first_run_util.h"
#include "brave/browser/day_zero_browser_ui_expt/day_zero_browser_ui_expt_manager.h"
#include "brave/build/android/jni_headers/DayZeroMojomHelper_jni.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/android/browser_context_handle.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace {
constexpr int kDayZeroFeatureDurationInDays = 1;
}  // namespace

namespace day_zero {

DayZeroBrowserUiExpt::DayZeroBrowserUiExpt(content::BrowserContext* context) {}

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
  DayZeroBrowserUiExpt* day_zero_browser_ui_expt =
      new DayZeroBrowserUiExpt(browser_context);
  return reinterpret_cast<intptr_t>(day_zero_browser_ui_expt);
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
  if (!base::FeatureList::IsEnabled(features::kBraveDayZeroExperiment)) {
    LOG(ERROR)
        << "NTP : "
        << "!base::FeatureList::IsEnabled(features::kBraveDayZeroExperiment)";
    return std::move(callback).Run(false);
  }

  LOG(ERROR) << "NTP : " << "IsDayZeroExpt 2";

  std::optional<std::string> day_zero_variant;
  if (base::FeatureList::IsEnabled(features::kBraveDayZeroExperiment)) {
    day_zero_variant = features::kBraveDayZeroExperimentVariant.Get();
  }

  LOG(ERROR) << "NTP : " << "IsDayZeroExpt 3";

  if (!day_zero_variant) {
    LOG(ERROR) << "NTP : " << "!day_zero_variant";
    return std::move(callback).Run(false);
  }
  LOG(ERROR) << "NTP : " << "IsDayZeroExpt 4";

  LOG(ERROR) << "NTP : " << "day_zero_variant : "
             << features::kBraveDayZeroExperimentVariant.Get();
  LOG(ERROR) << "NTP : " << "base::Time::Now() : " << base::Time::Now();
  LOG(ERROR) << "NTP : " << "brave_stats::GetFirstRunTime(pref_service_) : "
             << brave_stats::GetFirstRunTime(g_browser_process->local_state());
  LOG(ERROR) << "NTP : "
             << "base::Time::Now() - brave_stats::GetFirstRunTime(nullptr) : "
             << base::Time::Now() - brave_stats::GetFirstRunTime(
                                        g_browser_process->local_state());
  if (day_zero_variant == "a" &&
      (base::Time::Now() -
           brave_stats::GetFirstRunTime(g_browser_process->local_state()) >=
       base::Days(kDayZeroFeatureDurationInDays))) {
    return std::move(callback).Run(true);
  }
  return std::move(callback).Run(false);
}

}  // namespace day_zero
