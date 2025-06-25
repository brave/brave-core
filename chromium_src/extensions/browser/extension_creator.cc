/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/browser/extension_creator.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "components/crx_file/crx_creator.h"

namespace {

constexpr char kPublisherKeySwitch[] = "brave-extension-publisher-key";

// A second publisher key; Useful for when we prepare to rotate the key:
constexpr char kAltPublisherKeySwitch[] = "brave-extension-publisher-key-alt";

}  // namespace

#define BRAVE_CREATE_CRX(output_path, zip_path, signing_key)              \
  std::vector<crypto::keypair::PrivateKey> keys{signing_key};             \
  const auto* cmd = base::CommandLine::ForCurrentProcess();               \
  const char* switches[] = {kPublisherKeySwitch, kAltPublisherKeySwitch}; \
  for (const char* switch_name : switches) {                              \
    if (cmd->HasSwitch(switch_name)) {                                    \
      std::optional<crypto::keypair::PrivateKey> key =                    \
          ReadInputKey(cmd->GetSwitchValuePath(switch_name));             \
      if (!key)                                                           \
        return false; /* error_message_ was set by ReadInputKey() */      \
      keys.push_back(std::move(key).value());                             \
    }                                                                     \
  }                                                                       \
  result = crx_file::CreateWithMultipleKeys(output_path, zip_path,        \
                                            base::span(keys));

#include "src/extensions/browser/extension_creator.cc"

#undef BRAVE_CREATE_CRX
