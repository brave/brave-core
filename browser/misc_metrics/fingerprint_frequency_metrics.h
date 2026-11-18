/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_FINGERPRINT_FREQUENCY_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_FINGERPRINT_FREQUENCY_METRICS_H_

#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

class PrefRegistrySimple;
class PrefService;
class Profile;

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace misc_metrics {

inline constexpr char kCanvasFingerprintHistogramName[] =
    "Brave.Shields.FPInput.Canvas.2";
inline constexpr char kCombinedFingerprintHistogramName[] =
    "Brave.Shields.FPInput.Combined.2";
inline constexpr char kDeviceMemoryFingerprintHistogramName[] =
    "Brave.Shields.FPInput.DeviceMemory.2";
inline constexpr char kDevicePixelRatioFingerprintHistogramName[] =
    "Brave.Shields.FPInput.DevicePixelRatio.2";
inline constexpr char kFontsFingerprintHistogramName[] =
    "Brave.Shields.FPInput.Fonts.2";
inline constexpr char kHardwareConcurrencyFingerprintHistogramName[] =
    "Brave.Shields.FPInput.HardwareConcurrency.2";
inline constexpr char kLanguagesFingerprintHistogramName[] =
    "Brave.Shields.FPInput.Languages.2";
inline constexpr char kScreenAvailSizeFingerprintHistogramName[] =
    "Brave.Shields.FPInput.ScreenAvailSize.2";
inline constexpr char kScreenPixelDepthFingerprintHistogramName[] =
    "Brave.Shields.FPInput.ScreenPixelDepth.2";
inline constexpr char kScreenSizeFingerprintHistogramName[] =
    "Brave.Shields.FPInput.ScreenSize.2";
inline constexpr char kTimezoneFingerprintHistogramName[] =
    "Brave.Shields.FPInput.Timezone.2";
inline constexpr char kUserAgentFingerprintHistogramName[] =
    "Brave.Shields.FPInput.UserAgent.2";
inline constexpr char kWebAudioFingerprintHistogramName[] =
    "Brave.Shields.FPInput.WebAudio.2";
inline constexpr char kWebGLExtensionsFingerprintHistogramName[] =
    "Brave.Shields.FPInput.WebGLExtensions.2";
inline constexpr char kWebGLRendererFingerprintHistogramName[] =
    "Brave.Shields.FPInput.WebGLRenderer.2";
inline constexpr char kWebGLVendorFingerprintHistogramName[] =
    "Brave.Shields.FPInput.WebGLVendor.2";

// Collects and reports browser fingerprint input stability metrics over time.
// This is for an internal privacy study. Only the number of unique fingerprint
// input hashes per week is reported, and fingerprint values are never
// transmitted.
class FingerprintFrequencyMetrics : public content::WebContentsObserver {
 public:
  FingerprintFrequencyMetrics(PrefService* local_state, Profile* profile);
  ~FingerprintFrequencyMetrics() override;

  FingerprintFrequencyMetrics(const FingerprintFrequencyMetrics&) = delete;
  FingerprintFrequencyMetrics& operator=(const FingerprintFrequencyMetrics&) =
      delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);
  static void RegisterPrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoletePrefs(PrefService* local_state);

  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  void ReportAllMetrics();

  // Makes all scheduled executions bypass the renderer and use `results`
  // instead.
  void SetFakeRendererResultsForTesting(base::DictValue results);

  // Runs the script in the renderer immediately, bypassing the execution timer.
  // `callback` receives the resulting hashes.
  void ExecuteRendererForTesting(
      base::OnceCallback<void(base::DictValue)> callback);

 private:
  bool HasReportIntervalElapsed();
  void StartExecution();
  void RunScriptInRenderer();
  void HandleResult(base::Value result);
  void Cleanup();

  raw_ptr<PrefService> local_state_;
  std::unique_ptr<content::WebContents> web_contents_;
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> injector_;
  base::WallClockTimer renderer_timer_;
  base::WallClockTimer report_timer_;
  base::OneShotTimer timeout_timer_;

  std::optional<base::DictValue> fake_renderer_results_for_testing_;
  base::OnceCallback<void(base::DictValue)> result_callback_for_testing_;

  raw_ptr<Profile> profile_;
  base::WeakPtrFactory<FingerprintFrequencyMetrics> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_FINGERPRINT_FREQUENCY_METRICS_H_
