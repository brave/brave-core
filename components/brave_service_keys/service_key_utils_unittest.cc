// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_service_keys/service_key_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_service_keys {

TEST(BraveServicesUtilsUnittest, GetDigestHeader) {
  // Test vector is from
  // https://www.ietf.org/archive/id/draft-ietf-httpbis-digest-headers-04.html#section-10.4
  auto header = GetDigestHeader("{\"hello\": \"world\"}");
  EXPECT_EQ(header.first, "digest");
  EXPECT_EQ(header.second,
            "SHA-256=X48E9qOokqqrvdts8nOJRJN3OWDUoyWxBf7kbu9DBPE=");
}

}  // namespace brave_service_keys
