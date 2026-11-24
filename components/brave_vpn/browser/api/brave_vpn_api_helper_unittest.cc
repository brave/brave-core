/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/api/brave_vpn_api_helper.h"

#include <string>

#include "base/base64.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_vpn {

TEST(BraveVPNAPIHelperTest, TicketInfoTest) {
  base::DictValue ticket_value = GetValueWithTicketInfos(
      "brave-vpn@brave.com", "It's cool feature", "Love the Brave VPN!",
      "credential", "USA/Boston");

  const std::string expected_support_ticket =
      "Love the Brave VPN!\n\nsubscriber-credential: "
      "credential\npayment-validation-method: brave-premium\ntimezone: "
      "USA/Boston";
  EXPECT_THAT(ticket_value,
              base::test::IsJson(absl::StrFormat(
                  R"({
                    "email": "brave-vpn@brave.com",
                    "subject": "It's cool feature",
                    "support-ticket": "%s",
                    "partner-client-id": "com.brave.browser",
                    "subscriber-credential": "credential",
                    "payment-validation-method": "brave-premium"
                  })",
                  base::Base64Encode(expected_support_ticket))));
}

}  // namespace brave_vpn
