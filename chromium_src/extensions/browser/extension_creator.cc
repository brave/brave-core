/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/browser/extension_creator.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "components/crx_file/crx_creator.h"

namespace {
const char kPublisherKeySwitch[] = "brave-extension-publisher-key";
}  // namespace

#define BRAVE_CREATE_CRX(output_path, zip_path, signing_key)        \
  const auto* cmd = base::CommandLine::ForCurrentProcess();         \
  std::unique_ptr<crypto::RSAPrivateKey> publisher_key;             \
  if (cmd->HasSwitch(kPublisherKeySwitch)) {                        \
    publisher_key =                                                 \
        ReadInputKey(cmd->GetSwitchValuePath(kPublisherKeySwitch)); \
    if (!publisher_key)                                             \
      return false; /* error_message_ was set by ReadInputKey() */  \
  }                                                                 \
  result = crx_file::CreateWithPublisherKey(output_path, zip_path,  \
                                            signing_key, publisher_key.get());

#include "src/extensions/browser/extension_creator.cc"

#undef BRAVE_CREATE_CRX
