/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

namespace ads {

class AdsURLLoader {
 public:
  virtual ~AdsURLLoader() = default;

  virtual void Start() = 0;
  virtual uint64_t request_id() = 0;
};

}  // namespace ads
