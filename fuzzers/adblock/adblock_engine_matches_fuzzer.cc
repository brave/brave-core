/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <fuzzer/FuzzedDataProvider.h>

#include <iostream>
#include <string>

#include "base/files/file_path.h"
#include "base/i18n/icu_util.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"
#include "brave/fuzzers/adblock/adblock_fuzzer.pb.h"
#include "testing/libfuzzer/proto/lpm_interface.h"
#include "testing/libfuzzer/proto/url_proto_converter.h"

std::string ResourceTypeToString(adblock_fuzzer::ResourceType resource_type) {
  switch (resource_type) {
    case adblock_fuzzer::ResourceType::kMainFrame:
      return "main_frame";
    case adblock_fuzzer::ResourceType::kSubFrame:
      return "sub_frame";
    case adblock_fuzzer::ResourceType::kStylesheet:
      return "stylesheet";
    case adblock_fuzzer::ResourceType::kScript:
      return "script";
    case adblock_fuzzer::ResourceType::kFavicon:
    case adblock_fuzzer::ResourceType::kImage:
      return "image";
    case adblock_fuzzer::ResourceType::kFontResource:
      return "font";
    case adblock_fuzzer::ResourceType::kSubResource:
      return "other";
    case adblock_fuzzer::ResourceType::kObject:
      return "object";
    case adblock_fuzzer::ResourceType::kMedia:
      return "media";
    case adblock_fuzzer::ResourceType::kXhr:
      return "xhr";
    case adblock_fuzzer::ResourceType::kPing:
      return "ping";
    case adblock_fuzzer::ResourceType::kWorker:
      return "worker";
    case adblock_fuzzer::ResourceType::kSharedWorker:
      return "shared_worker";
    case adblock_fuzzer::ResourceType::kPrefetch:
      return "prefetch";
    case adblock_fuzzer::ResourceType::kServiceWorker:
      return "service_worker";
    case adblock_fuzzer::ResourceType::kCspReport:
      return "csp_report";
    case adblock_fuzzer::ResourceType::kPluginResource:
      return "plugin_resource";
    case adblock_fuzzer::ResourceType::kNavigationPreloadMainFrame:
      return "navigation_preload_main_frame";
    case adblock_fuzzer::ResourceType::kNavigationPreloadSubFrame:
      return "navigation_preload_sub_frame";
    default:
      break;
  }
  return {};
}

struct Environment {
  Environment() : engine(adblock::new_engine()) {
    CHECK(base::i18n::InitializeICU());
    auto result = engine->deserialize(brave_component_updater::ReadDATFileData(
        base::FilePath::FromASCII("rs-ABPFilterParserData.dat")));
    CHECK(result);
    CHECK(adblock::set_domain_resolver());
  }

  rust::Box<adblock::Engine> engine;
};

// Make sure 'rs-ABPFilterParserData.dat' file exists in the working directory
// before running the adblock_engine_matches_fuzzer executable.
DEFINE_PROTO_FUZZER(const adblock_fuzzer::EngineMatches& input) {
  static Environment env;
  const auto url = url_proto::Convert(input.url());
  if (::getenv("LPM_DUMP_NATIVE_INPUT")) {
    std::cout << url << std::endl;
  }

  env.engine->matches(url, (input.url().has_host() ? input.url().host() : url),
                      url_proto::Convert(input.tab_host()),
                      ResourceTypeToString(input.resource_type()),
                      input.is_third_party(), false, false);
}
