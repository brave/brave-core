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
#include "base/timer/timer.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "brave/components/misc_metrics/page_percentage_metrics.h"
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

inline constexpr char kCanvasFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.Canvas";
inline constexpr char kDeviceMemoryFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.DeviceMemory";
inline constexpr char kDevicePixelRatioFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.DevicePixelRatio";
inline constexpr char kFontsFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.Fonts";
inline constexpr char kHardwareConcurrencyFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.HardwareConcurrency";
inline constexpr char kLanguagesFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.Languages";
inline constexpr char kScreenAvailSizeFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.ScreenAvailSize";
inline constexpr char kScreenPixelDepthFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.ScreenPixelDepth";
inline constexpr char kScreenSizeFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.ScreenSize";
inline constexpr char kTimezoneFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.Timezone";
inline constexpr char kUserAgentFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.UserAgent";
inline constexpr char kWebAudioFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.WebAudio";
inline constexpr char kWebGLExtensionsFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.WebGLExtensions";
inline constexpr char kWebGLRendererFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.WebGLRenderer";
inline constexpr char kWebGLVendorFingerprintPercentHistogramName[] =
    "Brave.Shields.FPInput.WebGLVendor";

// Collects and reports browser fingerprint input stability metrics over time.
// This is for an internal privacy study. Only the frequency of fingerprint
// input changes are reported, and fingerprint values are never transmitted.
class FingerprintFrequencyMetrics : public PagePercentageMetrics,
                                    public content::WebContentsObserver {
 public:
  FingerprintFrequencyMetrics(PrefService* local_state, Profile* profile);
  ~FingerprintFrequencyMetrics() override;

  FingerprintFrequencyMetrics(const FingerprintFrequencyMetrics&) = delete;
  FingerprintFrequencyMetrics& operator=(const FingerprintFrequencyMetrics&) =
      delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

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
  void StartExecution();
  void RunScriptInRenderer();
  void HandleResult(base::Value result);
  void Cleanup();

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
