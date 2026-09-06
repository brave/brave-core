/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_COMMON_HISTOGRAM_NAMES_H_
#define BRAVE_COMPONENTS_MISC_METRICS_COMMON_HISTOGRAM_NAMES_H_

namespace misc_metrics {

// Captcha related histograms goes here.
inline constexpr char kCaptchaCountHistogramName[] = "Brave.CaptchaCount.Total";
inline constexpr char kCaptchaGoogleCountHistogramName[] =
    "Brave.CaptchaCount.Google";
inline constexpr char kCaptchaCloudflareCountHistogramName[] =
    "Brave.CaptchaCount.Cloudflare";
inline constexpr char kCaptchaHCaptchaCountHistogramName[] =
    "Brave.CaptchaCount.hCaptcha";

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_COMMON_HISTOGRAM_NAMES_H_
