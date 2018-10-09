/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <ctime>

namespace catalog {

class AdsServe {
 public:
  explicit AdsServe(const std::string& path);
  ~AdsServe();

  void set_ping(std::time_t ping);

  std::time_t NextCatalogCheck();

  bool DownloadCatalog();

 private:
  std::string url_;
  std::string path_;

  std::time_t next_catalog_check_;
  std::time_t ping_;
};

}  // namespace catalog
