/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/constants/brave_services_key.h"
#include "url/gurl.h"

namespace content {
namespace google_apis {
std::string GetAPIKey() {
  return BUILDFLAG(BRAVE_SERVICES_KEY);
}
}  // namespace google_apis
}  // namespace content

namespace {

BASE_FEATURE(kSttFeature, "speech_to_text", base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<std::string> kSttUrl{&kSttFeature, "web_service_url",
                                              ""};

std::string GetWebServiceBaseUrl(const char* web_service_base_url) {
  if (base::FeatureList::IsEnabled(kSttFeature)) {
    return kSttUrl.Get();
  }
  // Fallback to Google's url.
  return web_service_base_url;
}

}  // namespace

#define downstream_url(arg)                                   \
  downstream_url(GetWebServiceBaseUrl(web_service_base_url) + \
                 std::string(kDownstreamUrl) +                \
                 base::JoinString(downstream_args, "&"));

#define upstream_url(arg)                                   \
  upstream_url(GetWebServiceBaseUrl(web_service_base_url) + \
               std::string(kUpstreamUrl) +                  \
               base::JoinString(upstream_args, "&"))

#include "src/content/browser/speech/network_speech_recognition_engine_impl.cc"

#undef downstream_url
#undef upstream_url
