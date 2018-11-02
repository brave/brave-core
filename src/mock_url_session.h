/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "bat/ads/url_session.h"

namespace ads {

class MockURLSession : public URLSession {
 public:
  MockURLSession();
  ~MockURLSession() override;

 protected:
  void Start() override;
  uint64_t GetSessionId() override;
};

}  // namespace ads
