/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SIGNATURE_BASENAME_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SIGNATURE_BASENAME_H_

#include <optional>
#include <vector>

#include "base/values.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"

class PrefService;

namespace web_discovery {

struct BasenameResult {
  BasenameResult(std::vector<uint8_t> basename,
                 size_t count,
                 uint32_t count_tag_hash);
  ~BasenameResult();

  BasenameResult(const BasenameResult&) = delete;
  BasenameResult& operator=(const BasenameResult&) = delete;

  std::vector<uint8_t> basename;
  // The count index for a given "pre-tag". It should be under the limit for a
  // given action
  size_t count;
  uint32_t count_tag_hash;
};

// Generates a basename used for the signature. The basename is a sha hash
// of the message "action" (i.e. "query"), the settings for that action
// (defined in the server's "source map"), cherry-picked attributes from the
// payload and the count index for the given message. The count will be under
// the limit defined for the action; the function will return nullopt if the
// limit for the action is exceeded.
std::optional<BasenameResult> GenerateBasename(
    PrefService* profile_prefs,
    const ServerConfig& server_config,
    const base::DictValue& payload);

// Saves the count returned from `GenerateBasename` in the prefs.
// This ensures that the count index cannot be used for future messages
// within the defined action limit period (default is 24 hours).
// This should be called after a submission is successfully sent to
// the server.
void SaveBasenameCount(PrefService* profile_prefs,
                       uint32_t count_tag_hash,
                       size_t count);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_SIGNATURE_BASENAME_H_
