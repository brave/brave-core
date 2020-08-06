/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_TOR_TOR_COMMON_H_
#define BRAVE_COMMON_TOR_TOR_COMMON_H_

#include "base/files/file_path.h"

namespace tor {

class TorConfig {
 public:
  TorConfig();
  explicit TorConfig(const base::FilePath& binary_path);
  TorConfig(const TorConfig& that);
  ~TorConfig();

  bool empty() const;
  const base::FilePath& binary_path() const { return binary_path_; }
  const base::FilePath& tor_data_path() const { return tor_data_path_; }
  const base::FilePath& tor_watch_path() const { return tor_watch_path_; }

 private:
  base::FilePath binary_path_;
  base::FilePath tor_data_path_;
  base::FilePath tor_watch_path_;
};

}  // namespace tor

#endif  // BRAVE_COMMON_TOR_TOR_COMMON_H_
