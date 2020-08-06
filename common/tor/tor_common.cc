/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_common.h"

#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"

namespace tor {

TorConfig::TorConfig() {}

TorConfig::TorConfig(const base::FilePath& binary_path)
  : binary_path_(binary_path) {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());
  tor_data_path_ = user_data_dir.Append(FILE_PATH_LITERAL("tor"))
    .Append(FILE_PATH_LITERAL("data"));
  tor_watch_path_ = user_data_dir.Append(FILE_PATH_LITERAL("tor"))
    .Append(FILE_PATH_LITERAL("watch"));
}

TorConfig::TorConfig(const TorConfig& that) = default;

TorConfig::~TorConfig() {}

bool TorConfig::empty() const {
  if (binary_path_.empty())
    return true;
  return false;
}

}  // namespace tor
