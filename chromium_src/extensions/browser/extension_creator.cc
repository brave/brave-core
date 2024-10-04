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
  std::vector<crypto::RSAPrivateKey*> keys{signing_key};                  \
  std::vector<std::unique_ptr<crypto::RSAPrivateKey>> keep_keys_alive;    \
  const auto* cmd = base::CommandLine::ForCurrentProcess();               \
  const char* switches[] = {kPublisherKeySwitch, kAltPublisherKeySwitch}; \
  for (const char* switch_name : switches) {                              \
    if (cmd->HasSwitch(switch_name)) {                                    \
      std::unique_ptr<crypto::RSAPrivateKey> key =                        \
          ReadInputKey(cmd->GetSwitchValuePath(switch_name));             \
      if (!key)                                                           \
        return false; /* error_message_ was set by ReadInputKey() */      \
      keys.push_back(key.get());                                          \
      keep_keys_alive.push_back(std::move(key));                          \
    }                                                                     \
  }                                                                       \
  result = crx_file::CreateWithMultipleKeys(output_path, zip_path, keys);

#include "src/extensions/browser/extension_creator.cc"

#undef BRAVE_CREATE_CRX
