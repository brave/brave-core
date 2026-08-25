/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/fingerprint_input_metrics.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/features.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
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
constexpr base::TimeDelta kCheckInterval = base::Minutes(30);
constexpr base::TimeDelta kStartDelay = base::Seconds(30);
constexpr base::TimeDelta kStartDelayMinRandom = base::Seconds(-5);
constexpr base::TimeDelta kStartDelayMaxRandom = base::Seconds(15);
constexpr base::TimeDelta kIntervalDelayMinRandom = base::Seconds(1);
constexpr base::TimeDelta kIntervalDelayMaxRandom = base::Seconds(120);
constexpr base::TimeDelta kExecutionTimeout = base::Seconds(30);

constexpr char kTotalCountKey[] = "total";

struct FingerprintMetricEntry {
  std::string_view count_key;
  const char* histogram_name;
  bool report_if_zero = false;
};

constexpr FingerprintMetricEntry kFingerprintMetrics[] = {
    {"canvas", kCanvasFingerprintPercentHistogramName},
    {"fonts", kFontsFingerprintPercentHistogramName},
    {"timezone", kTimezoneFingerprintPercentHistogramName},
    {"navigator_deviceMemory", kDeviceMemoryFingerprintPercentHistogramName},
    {"navigator_hardwareConcurrency",
     kHardwareConcurrencyFingerprintPercentHistogramName},
    {"navigator_languages", kLanguagesFingerprintPercentHistogramName},
    {"navigator_userAgent", kUserAgentFingerprintPercentHistogramName},
    {"screenAvailSize", kScreenAvailSizeFingerprintPercentHistogramName},
    {"screenSize", kScreenSizeFingerprintPercentHistogramName, true},
    {"screen_pixelDepth", kScreenPixelDepthFingerprintPercentHistogramName},
    {"webAudio", kWebAudioFingerprintPercentHistogramName},
    {"webglExtensions", kWebGLExtensionsFingerprintPercentHistogramName},
    {"webglRendererUnmasked", kWebGLRendererFingerprintPercentHistogramName},
    {"webglVendorUnmasked", kWebGLVendorFingerprintPercentHistogramName},
    {"windowDevicePixelRatio",
     kDevicePixelRatioFingerprintPercentHistogramName},
};

}  // namespace

// static
void FingerprintInputMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsFingerprintHashes);
  registry->RegisterDictionaryPref(kMiscMetricsFingerprintChangeCounts);
  registry->RegisterTimePref(kMiscMetricsFingerprintReportFrameStartTime, {});
  registry->RegisterTimePref(kMiscMetricsFingerprintLastExecutionTime, {});
}

FingerprintInputMetrics::FingerprintInputMetrics(PrefService* local_state,
                                                 Profile* profile)
    : PagePercentageMetrics(local_state,
                            kMiscMetricsFingerprintChangeCounts,
                            kMiscMetricsFingerprintReportFrameStartTime,
                            kReportInterval),
      content::WebContentsObserver(nullptr),
      profile_(profile) {
  ReportAllMetrics();
  base::TimeDelta delay =
      kStartDelay +
      base::RandTimeDelta(kStartDelayMinRandom, kStartDelayMaxRandom);
  renderer_timer_.Start(FROM_HERE, base::Time::Now() + delay, this,
                        &FingerprintInputMetrics::StartExecution);
}

FingerprintInputMetrics::~FingerprintInputMetrics() = default;

void FingerprintInputMetrics::ReportAllMetrics() {
  base::Time now = base::Time::Now();
  report_timer_.Start(FROM_HERE, now + kCheckInterval, this,
                      &FingerprintInputMetrics::ReportAllMetrics);

  if (!HasReportIntervalElapsed()) {
    return;
  }

  const base::DictValue& counts =
      local_state_->GetDict(kMiscMetricsFingerprintChangeCounts);
  int total = counts.FindInt(kTotalCountKey).value_or(0);
  if (total > 0) {
    for (const auto& metric : kFingerprintMetrics) {
      RecordPercentageHistogram(counts, total, metric.count_key,
                                metric.histogram_name, metric.report_if_zero);
    }
  }

  ResetCounts();
}

void FingerprintInputMetrics::SetFakeRendererResultsForTesting(
    base::DictValue results) {
  fake_renderer_results_ = std::move(results);
}

void FingerprintInputMetrics::ExecuteRendererForTesting(
    base::OnceCallback<void(base::DictValue)> callback) {
  result_callback_for_testing_ = std::move(callback);
  RunScriptInRenderer();
}

void FingerprintInputMetrics::StartExecution() {
  base::Time now = base::Time::Now();
  renderer_timer_.Start(
      FROM_HERE,
      now + kCheckInterval +
          base::RandTimeDelta(kIntervalDelayMinRandom, kIntervalDelayMaxRandom),
      this, &FingerprintInputMetrics::StartExecution);

  base::Time last_execution =
      local_state_->GetTime(kMiscMetricsFingerprintLastExecutionTime);
  if (!last_execution.is_null() &&
      now - last_execution <
          features::kFingerprintInputRendererInterval.Get()) {
    return;
  }

  local_state_->SetTime(kMiscMetricsFingerprintLastExecutionTime, now);

  if (fake_renderer_results_) {
    HandleResult(base::Value(fake_renderer_results_->Clone()));
    return;
  }

  RunScriptInRenderer();
}

void FingerprintInputMetrics::RunScriptInRenderer() {
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
                       base::BindOnce(&FingerprintInputMetrics::Cleanup,
                                      base::Unretained(this)));
}

void FingerprintInputMetrics::DidFinishLoad(
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
      base::BindOnce(&FingerprintInputMetrics::HandleResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FingerprintInputMetrics::HandleResult(base::Value result) {
  timeout_timer_.Stop();

  if (!result.is_dict()) {
    Cleanup();
    return;
  }

  const base::DictValue& new_hashes = result.GetDict();
  const base::DictValue& previous_hashes =
      local_state_->GetDict(kMiscMetricsFingerprintHashes);

  IncrementDictCount(kTotalCountKey);

  if (!previous_hashes.empty()) {
    for (const auto [key, new_val] : new_hashes) {
      const base::Value* prev_val = previous_hashes.Find(key);
      if (prev_val && *prev_val != new_val) {
        IncrementDictCount(key);
      }
    }
  }

  local_state_->SetDict(kMiscMetricsFingerprintHashes, new_hashes.Clone());
  Cleanup();

  if (result_callback_for_testing_) {
    std::move(result_callback_for_testing_).Run(new_hashes.Clone());
  }
}

void FingerprintInputMetrics::Cleanup() {
  Observe(nullptr);
  injector_.reset();
  if (web_contents_) {
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(
        FROM_HERE, std::move(web_contents_));
  }
}

}  // namespace misc_metrics
