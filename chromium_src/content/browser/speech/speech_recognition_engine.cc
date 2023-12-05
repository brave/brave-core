/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/command_line.h"

namespace brave_chromium_src {

constexpr const char kSpeechWebService[] = "speech-web-service-url";
std::string GetWebServiceBaseUrl();

}  // namespace brave_chromium_src

#define downstream_url(arg)                                   \
  downstream_url(brave_chromium_src::GetWebServiceBaseUrl() + \
                 std::string(kDownstreamUrl) +                \
                 base::JoinString(downstream_args, "&"));     \
  web_service_base_url;  // unused

#define upstream_url(arg)                                   \
  upstream_url(brave_chromium_src::GetWebServiceBaseUrl() + \
               std::string(kDownstreamUrl) +                \
               base::JoinString(downstream_args, "&"))

#include "src/content/browser/speech/speech_recognition_engine.cc"

#undef downstream_url
#undef upstream_url

namespace brave_chromium_src {

std::string GetWebServiceBaseUrl() {
  auto* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line && cmd_line->HasSwitch(kSpeechWebService)) {
    return cmd_line->GetSwitchValueASCII(kSpeechWebService);
  }

  if (content::web_service_base_url_for_tests) {
    return content::web_service_base_url_for_tests;
  }
  return content::kWebServiceBaseUrl;
}

}  // namespace brave_chromium_src
