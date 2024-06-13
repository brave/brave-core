/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SIGNATURE_BASENAME_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SIGNATURE_BASENAME_H_

#include <optional>
#include <vector>

#include "base/values.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"

class PrefService;

namespace web_discovery {

struct BasenameResult {
  BasenameResult(std::vector<const uint8_t> basename,
                 size_t count,
                 uint32_t count_tag_hash);
  ~BasenameResult();

  BasenameResult(const BasenameResult&) = delete;
  BasenameResult& operator=(const BasenameResult&) = delete;

  std::vector<const uint8_t> basename;
  // The count index for a given "pre-tag". It should be under the limit for a
  // given action
  size_t count;
  uint32_t count_tag_hash;
};

std::optional<BasenameResult> GenerateBasename(
    PrefService* profile_prefs,
    ServerConfig* server_config,
    RegexUtil& regex_util,
    const base::Value::Dict& payload);

void SaveBasenameCount(PrefService* profile_prefs,
                       uint32_t count_tag_hash,
                       size_t count);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PAYLOAD_GENERATOR_H_
