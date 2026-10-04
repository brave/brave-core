/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/api/brave_vpn_api_helper.h"

#include "base/base64.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

using v1::endpoints::kSupportTicketEmailKey;
using v1::endpoints::kSupportTicketPartnerClientIdKey;
using v1::endpoints::kSupportTicketPaymentValidationMethodKey;
using v1::endpoints::kSupportTicketSubjectKey;
using v1::endpoints::kSupportTicketSubscriberCredential;
using v1::endpoints::kSupportTicketSupportTicketKey;

TEST(BraveVPNAPIHelperTest, TicketInfoTest) {
  base::DictValue ticket_value = GetValueWithTicketInfos(
      "brave-vpn@brave.com", "It's cool feature", "Love the Brave VPN!",
      "credential", "USA/Boston");

  // Check ticket dict has four required fields.
  EXPECT_TRUE(ticket_value.FindString(kSupportTicketEmailKey));
  EXPECT_TRUE(ticket_value.FindString(kSupportTicketSubjectKey));
  EXPECT_TRUE(ticket_value.FindString(kSupportTicketPartnerClientIdKey));
  const auto support_ticket_encoded =
      *ticket_value.FindString(kSupportTicketSupportTicketKey);
  EXPECT_TRUE(!support_ticket_encoded.empty());
  auto* subscriber_credential =
      ticket_value.FindString(kSupportTicketSubscriberCredential);
  ASSERT_TRUE(subscriber_credential);
  EXPECT_EQ(*subscriber_credential, "credential");
  auto* payment_validation_method =
      ticket_value.FindString(kSupportTicketPaymentValidationMethodKey);
  ASSERT_TRUE(payment_validation_method);
  EXPECT_EQ(*payment_validation_method, "brave-premium");

  // Check body contents
  std::string support_ticket_decoded;
  EXPECT_TRUE(
      base::Base64Decode(support_ticket_encoded, &support_ticket_decoded));
  const std::string expected_support_ticket =
      "Love the Brave VPN!\n\nsubscriber-credential: "
      "credential\npayment-validation-method: brave-premium\ntimezone: "
      "USA/Boston";
  EXPECT_EQ(expected_support_ticket, support_ticket_decoded);
}

}  // namespace brave_vpn
