/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_MOCK_IMPORTER_BRIDGE_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_MOCK_IMPORTER_BRIDGE_H_

#include <vector>

#include "chrome/common/importer/mock_importer_bridge.h"
#include "net/cookies/canonical_cookie.h"
#include "testing/gmock/include/gmock/gmock.h"

class BraveMockImporterBridge : public MockImporterBridge {
 public:
  BraveMockImporterBridge();

  MOCK_METHOD1(SetCookies,
               void(const std::vector<net::CanonicalCookie>&));

 private:
  ~BraveMockImporterBridge() override;
};

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_MOCK_IMPORTER_BRIDGE_H_
