/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cinttypes>

#include "mock_url_session.h"

namespace ads {

MockURLSession::MockURLSession() = default;

MockURLSession::~MockURLSession() = default;

void MockURLSession::Start() {
}

uint64_t MockURLSession::GetSessionId() const {
  return 0;
}

}  // namespace ads
