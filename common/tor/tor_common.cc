/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_common.h"

#include <algorithm>
#include <limits>
#include <string>

#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "url/third_party/mozilla/url_parse.h"

namespace tor {

TorConfig::TorConfig() {}

TorConfig::TorConfig(const base::FilePath& binary_path,
                     const std::string& proxy_string)
  : binary_path_(binary_path),
    proxy_string_(proxy_string) {
  if (proxy_string.length()) {
    url::Parsed url;
    url::ParseStandardURL(
      proxy_string.c_str(),
      std::min(proxy_string.size(),
               static_cast<size_t>(std::numeric_limits<int>::max())),
      &url);
    if (url.host.is_valid()) {
      proxy_host_ =
        std::string(proxy_string.begin() + url.host.begin,
                    proxy_string.begin() + url.host.begin + url.host.len);
    }
    if (url.port.is_valid()) {
      proxy_port_ =
        std::string(proxy_string.begin() + url.port.begin,
                    proxy_string.begin() + url.port.begin + url.port.len);
    }
  }

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
  if (binary_path_.empty() || proxy_string_.empty())
    return true;
  return false;
}

}  // namespace tor
