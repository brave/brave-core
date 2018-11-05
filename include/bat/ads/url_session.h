/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "bat/ads/export.h"

namespace ads {

class ADS_EXPORT URLSession {
 public:
  enum Method {
    GET = 0,
    PUT = 1,
    POST = 2
  };

  virtual ~URLSession() = default;

  virtual void Start() = 0;
  virtual uint64_t GetSessionId() const = 0;
};

}  // namespace ads
