/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/api/support_endpoints.h"

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::endpoints {
namespace {
constexpr char kTestEmail[] = "user@example.com";
constexpr char kTestSubject[] = "Help";
constexpr char kTestBody[] = "It doesn't connect.";
constexpr char kTestSubscriberCredential[] = "test-subscriber-credential";
constexpr char kTestTimezone[] = "America/Los_Angeles";
}  // namespace

TEST(SupportEndpointsTest, CreateSupportTicketRequestBodyToValue) {
  const CreateSupportTicketRequestBody body{
      .email = kTestEmail,
      .subject = kTestSubject,
      .body = kTestBody,
      .subscriber_credential = kTestSubscriberCredential,
      .timezone = kTestTimezone};

  // The encoded ticket body embeds credential, validation method, and
  // timezone as text lines, in addition to the separate JSON fields above.
  const std::string expected_encoded_body = base::Base64Encode(base::StrCat(
      {kTestBody, "\n\nsubscriber-credential: ", kTestSubscriberCredential,
       "\npayment-validation-method: brave-premium\ntimezone: ",
       kTestTimezone}));

  EXPECT_EQ(body.ToValue(),
            base::DictValue()
                .Set("email", kTestEmail)
                .Set("subject", kTestSubject)
                .Set("support-ticket", expected_encoded_body)
                .Set("partner-client-id", "com.brave.browser")
                .Set("payment-validation-method", "brave-premium")
                .Set("subscriber-credential", kTestSubscriberCredential));
}

}  // namespace brave_vpn::v2::endpoints
