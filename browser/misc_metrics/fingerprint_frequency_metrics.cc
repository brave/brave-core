/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/fingerprint_frequency_metrics.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/features.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/isolated_world_ids.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/renderer_preferences/renderer_preferences.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Days(7);
constexpr base::TimeDelta kStartDelay = base::Seconds(30);
constexpr base::TimeDelta kExecutionTimeout = base::Seconds(30);

constexpr int kUniqueHashBuckets[] = {0, 1, 5, 11, 15};

struct FingerprintMetricEntry {
  std::string_view hash_key;
  const char* histogram_name;
  bool is_combined = false;
};

constexpr FingerprintMetricEntry kFingerprintMetrics[] = {
    {"canvas", kCanvasFingerprintHistogramName},
    {"combined", kCombinedFingerprintHistogramName, true},
    {"fonts", kFontsFingerprintHistogramName},
    {"timezone", kTimezoneFingerprintHistogramName},
    {"navigator_deviceMemory", kDeviceMemoryFingerprintHistogramName},
    {"navigator_hardwareConcurrency",
     kHardwareConcurrencyFingerprintHistogramName},
    {"navigator_languages", kLanguagesFingerprintHistogramName},
    {"navigator_userAgent", kUserAgentFingerprintHistogramName},
    {"screenAvailSize", kScreenAvailSizeFingerprintHistogramName},
    {"screenSize", kScreenSizeFingerprintHistogramName},
    {"screen_pixelDepth", kScreenPixelDepthFingerprintHistogramName},
    {"webAudio", kWebAudioFingerprintHistogramName},
    {"webglExtensions", kWebGLExtensionsFingerprintHistogramName},
    {"webglRendererUnmasked", kWebGLRendererFingerprintHistogramName},
    {"webglVendorUnmasked", kWebGLVendorFingerprintHistogramName},
    {"windowDevicePixelRatio", kDevicePixelRatioFingerprintHistogramName},
};

}  // namespace

// static
void FingerprintFrequencyMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsFingerprintHashes);
  registry->RegisterTimePref(kMiscMetricsFingerprintReportFrameStartTime, {});
  registry->RegisterTimePref(kMiscMetricsFingerprintLastExecutionTime, {});
}

// static
void FingerprintFrequencyMetrics::RegisterPrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 11/2026
  registry->RegisterDictionaryPref(kMiscMetricsFingerprintChangeCounts);
}

// static
void FingerprintFrequencyMetrics::MigrateObsoletePrefs(
    PrefService* local_state) {
  // Added 11/2026
  local_state->ClearPref(kMiscMetricsFingerprintChangeCounts);
}

FingerprintFrequencyMetrics::FingerprintFrequencyMetrics(
    PrefService* local_state,
    Profile* profile)
    : content::WebContentsObserver(nullptr),
      local_state_(local_state),
      profile_(profile) {
  ReportAllMetrics();
  renderer_timer_.Start(FROM_HERE, base::Time::Now() + kStartDelay, this,
                        &FingerprintFrequencyMetrics::StartExecution);
}

FingerprintFrequencyMetrics::~FingerprintFrequencyMetrics() = default;

void FingerprintFrequencyMetrics::ReportAllMetrics() {
  base::Time now = base::Time::Now();
  if (!HasReportIntervalElapsed()) {
    base::Time frame_start =
        local_state_->GetTime(kMiscMetricsFingerprintReportFrameStartTime);
    report_timer_.Start(FROM_HERE, frame_start + kReportInterval, this,
                        &FingerprintFrequencyMetrics::ReportAllMetrics);
    return;
  }

  const base::DictValue& hashes =
      local_state_->GetDict(kMiscMetricsFingerprintHashes);
  for (const auto& metric : kFingerprintMetrics) {
    const base::ListValue* hash_list = hashes.FindList(metric.hash_key);
    if (!hash_list) {
      continue;
    }
    int unique_count = static_cast<int>(hash_list->size());
    // Only the combined metric reports the 1 bucket. For other metrics, the 1
    // bucket is inferred virtually by subtracting their total from the total
    // of the combined metric.
    if (unique_count < (metric.is_combined ? 1 : 2)) {
      continue;
    }
    p3a_utils::RecordToHistogramBucket(metric.histogram_name,
                                       kUniqueHashBuckets, unique_count);
  }

  local_state_->ClearPref(kMiscMetricsFingerprintHashes);
  local_state_->SetTime(kMiscMetricsFingerprintReportFrameStartTime, now);
  report_timer_.Start(FROM_HERE, now + kReportInterval, this,
                      &FingerprintFrequencyMetrics::ReportAllMetrics);
}

void FingerprintFrequencyMetrics::SetFakeRendererResultsForTesting(
    base::DictValue results) {
  fake_renderer_results_for_testing_ = std::move(results);
}

void FingerprintFrequencyMetrics::ExecuteRendererForTesting(
    base::OnceCallback<void(base::DictValue)> callback) {
  result_callback_for_testing_ = std::move(callback);
  RunScriptInRenderer();
}

bool FingerprintFrequencyMetrics::HasReportIntervalElapsed() {
  base::Time frame_start =
      local_state_->GetTime(kMiscMetricsFingerprintReportFrameStartTime);
  if (frame_start.is_null()) {
    local_state_->SetTime(kMiscMetricsFingerprintReportFrameStartTime,
                          base::Time::Now());
    return false;
  }
  return base::Time::Now() - frame_start >= kReportInterval;
}

void FingerprintFrequencyMetrics::StartExecution() {
  base::Time now = base::Time::Now();
  base::Time last_execution =
      local_state_->GetTime(kMiscMetricsFingerprintLastExecutionTime);
  base::TimeDelta interval = features::kFingerprintInputRendererInterval.Get();
  if (!last_execution.is_null() && (now - last_execution) < interval) {
    renderer_timer_.Start(FROM_HERE, last_execution + interval, this,
                          &FingerprintFrequencyMetrics::StartExecution);
    return;
  }

  local_state_->SetTime(kMiscMetricsFingerprintLastExecutionTime, now);
  renderer_timer_.Start(FROM_HERE, now + interval, this,
                        &FingerprintFrequencyMetrics::StartExecution);

  if (fake_renderer_results_for_testing_) {
    HandleResult(base::Value(fake_renderer_results_for_testing_->Clone()));
    return;
  }

  RunScriptInRenderer();
}

void FingerprintFrequencyMetrics::RunScriptInRenderer() {
  if (web_contents_) {
    return;
  }

  content::WebContents::CreateParams create_params(profile_);
  web_contents_ = content::WebContents::Create(create_params);
  Observe(web_contents_.get());

  const std::string& accept_languages =
      profile_->GetPrefs()->GetString(language::prefs::kAcceptLanguages);
  if (!accept_languages.empty()) {
    web_contents_->GetMutableRendererPrefs()->accept_languages =
        accept_languages;
    web_contents_->SyncRendererPrefs();
  }

  web_contents_->GetController().LoadURL(
      GURL(chrome::kChromeUIVersionURL), content::Referrer(),
      ui::PAGE_TRANSITION_TYPED, std::string());

  timeout_timer_.Start(FROM_HERE, kExecutionTimeout,
                       base::BindOnce(&FingerprintFrequencyMetrics::Cleanup,
                                      base::Unretained(this)));
}

void FingerprintFrequencyMetrics::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (render_frame_host->GetParent() || injector_.is_bound()) {
    return;
  }

  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&injector_);

  std::string script =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_MISC_METRICS_FINGERPRINT_STABILITY_JS);

  injector_->RequestAsyncExecuteScript(
      content::ISOLATED_WORLD_ID_GLOBAL, base::UTF8ToUTF16(script),
      blink::mojom::UserActivationOption::kDoNotActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(&FingerprintFrequencyMetrics::HandleResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FingerprintFrequencyMetrics::HandleResult(base::Value result) {
  timeout_timer_.Stop();

  if (!result.is_dict()) {
    Cleanup();
    return;
  }

  const base::DictValue& new_hashes = result.GetDict();
  {
    ScopedDictPrefUpdate update(local_state_, kMiscMetricsFingerprintHashes);
    for (const auto [key, new_val] : new_hashes) {
      if (!new_val.is_int()) {
        continue;
      }
      base::ListValue* hash_list = update->EnsureList(key);
      if (!hash_list->contains(new_val.GetInt())) {
        hash_list->Append(new_val.GetInt());
      }
    }
  }

  Cleanup();

  if (result_callback_for_testing_) {
    std::move(result_callback_for_testing_).Run(new_hashes.Clone());
  }
}

void FingerprintFrequencyMetrics::Cleanup() {
  Observe(nullptr);
  injector_.reset();
  if (web_contents_) {
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(web_contents_));
  }
}

}  // namespace misc_metrics
