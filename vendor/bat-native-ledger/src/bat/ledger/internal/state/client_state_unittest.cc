/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <map>

#include "bat/ledger/internal/state/client_state.h"
#include "base/base64.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ClientStateTest.*

namespace ledger {

class ClientStateTest : public ::testing::Test {
 protected:
  ClientStateTest() {
    // You can do set-up work for each test here
  }

  ~ClientStateTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  WalletInfoProperties GetWalletInfoProperties() const {
    WalletInfoProperties wallet_info_properties;
    wallet_info_properties.payment_id = "PaymentId";
    wallet_info_properties.address_card_id = "AddressCardId";

    std::string key_info_seed;
    std::string base64_key_info_seed =
        "/kBv0C7wS4EBY3EIa780pYLrhryP3IWCfElIehufOFw=";
    base::Base64Decode(base64_key_info_seed, &key_info_seed);
    wallet_info_properties.key_info_seed.assign(key_info_seed.begin(),
        key_info_seed.end());

    return wallet_info_properties;
  }

  WalletProperties GetWalletProperties() const {
    WalletProperties wallet_properties;
    wallet_properties.fee_amount = std::numeric_limits<double>::max();
    wallet_properties.parameters_choices = {
      5.0,
      10.0,
      15.0,
      20.0,
      25.0,
      50.0,
      100.0
    };

    return wallet_properties;
  }

  std::map<std::string, bool> GetInlineTips(
      const int count) const {
    std::map<std::string, bool> inline_tips;
    for (int i = 0; i < count; i++) {
      const std::string key = std::to_string(i);
      inline_tips.insert({key, true});
    }

    return inline_tips;
  }
};

TEST_F(ClientStateTest, ToJsonSerializationWithMinValues) {
  // Arrange
  ClientProperties client_properties;
  client_properties.wallet_info = GetWalletInfoProperties();
  client_properties.wallet = GetWalletProperties();
  client_properties.boot_timestamp =
      std::numeric_limits<uint32_t>::min();
  client_properties.reconcile_timestamp =
      std::numeric_limits<uint32_t>::min();
  client_properties.persona_id = "PersonaId";
  client_properties.user_id = "UserId";
  client_properties.registrar_vk = "RegistrarVk";
  client_properties.pre_flight = "PreFlight";
  client_properties.fee_amount = std::numeric_limits<double>::min();
  client_properties.user_changed_fee = true;
  client_properties.auto_contribute = true;
  client_properties.rewards_enabled = true;
  client_properties.inline_tips = GetInlineTips(1);

  // Act
  const ClientState client_state;
  const std::string json = client_state.ToJson(client_properties);

  // Assert
  ClientProperties expected_client_properties;
  client_state.FromJson(json, &expected_client_properties);
  EXPECT_EQ(expected_client_properties, client_properties);
}

TEST_F(ClientStateTest, FromJsonDeserializationWithMinValues) {
  // Arrange
  ClientProperties client_properties;
  client_properties.wallet_info = GetWalletInfoProperties();
  client_properties.wallet = GetWalletProperties();
  client_properties.boot_timestamp =
      std::numeric_limits<uint32_t>::min();
  client_properties.reconcile_timestamp =
      std::numeric_limits<uint32_t>::min();
  client_properties.persona_id = "PersonaId";
  client_properties.user_id = "UserId";
  client_properties.registrar_vk = "RegistrarVk";
  client_properties.pre_flight = "PreFlight";
  client_properties.fee_amount = std::numeric_limits<double>::min();
  client_properties.user_changed_fee = true;
  client_properties.auto_contribute = true;
  client_properties.rewards_enabled = true;
  client_properties.inline_tips = GetInlineTips(1);

  const std::string json = "{\"walletInfo\":{\"paymentId\":\"PaymentId\",\"addressCARD_ID\":\"AddressCardId\",\"keyInfoSeed\":\"/kBv0C7wS4EBY3EIa780pYLrhryP3IWCfElIehufOFw=\"},\"bootStamp\":0,\"reconcileStamp\":0,\"personaId\":\"PersonaId\",\"userId\":\"UserId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"preFlight\":\"PreFlight\",\"fee_currency\":\"FeeCurrency\",\"settings\":\"Settings\",\"fee_amount\":2.2250738585072014e-308,\"user_changed_fee\":true,\"days\":0,\"rewards_enabled\":true,\"auto_contribute\":true,\"transactions\":[{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"contribution_probi\":\"ContributionProbi\",\"submissionStamp\":\"SubmissionTimestamp\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"surveyorIds\":[\"SurveyorId\"],\"votes\":4294967295,\"ballots\":[{\"publisher\":\"Publisher\",\"offset\":4294967295}]}],\"ballots\":[{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"}],\"batch\":[{\"publisher\":\"Publisher\",\"batchVotesInfo\":[{\"surveyorId\":\"SurveyorId\",\"proof\":\"Proof\"}]}],\"current_reconciles\":{\"0\":{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"}},\"walletProperties\":{\"fee_amount\":1.7976931348623157e308,\"parameters\":{\"adFree\":{\"fee\":{\"BAT\":1.7976931348623157e308},\"choices\":{\"BAT\":[5.0,10.0,15.0,20.0,25.0,50.0,100.0]}}}},\"inlineTip\":{\"0\":true}}";  // NOLINT

  // Act
  ClientProperties expected_client_properties;
  const ClientState client_state;
  client_state.FromJson(json, &expected_client_properties);

  // Assert
  EXPECT_EQ(expected_client_properties, client_properties);
}

TEST_F(ClientStateTest, ToJsonSerializationWithMaxValues) {
  // Arrange
  ClientProperties client_properties;
  client_properties.wallet_info = GetWalletInfoProperties();
  client_properties.wallet = GetWalletProperties();
  client_properties.boot_timestamp =
      std::numeric_limits<uint32_t>::max();
  client_properties.reconcile_timestamp =
      std::numeric_limits<uint32_t>::max();
  client_properties.persona_id = "PersonaId";
  client_properties.user_id = "UserId";
  client_properties.registrar_vk = "RegistrarVk";
  client_properties.pre_flight = "PreFlight";
  client_properties.fee_amount = std::numeric_limits<double>::max();
  client_properties.user_changed_fee = true;
  client_properties.auto_contribute = true;
  client_properties.rewards_enabled = true;
  client_properties.inline_tips = GetInlineTips(9);

  // Act
  const ClientState client_state;
  const std::string json = client_state.ToJson(client_properties);

  // Assert
  ClientProperties expected_client_properties;
  client_state.FromJson(json, &expected_client_properties);
  EXPECT_EQ(expected_client_properties, client_properties);
}

TEST_F(ClientStateTest, FromJsonDeserializationWithMaxValues) {
  // Arrange
  ClientProperties client_properties;
  client_properties.wallet_info = GetWalletInfoProperties();
  client_properties.wallet = GetWalletProperties();
  client_properties.boot_timestamp =
      std::numeric_limits<uint32_t>::max();
  client_properties.reconcile_timestamp =
      std::numeric_limits<uint32_t>::max();
  client_properties.persona_id = "PersonaId";
  client_properties.user_id = "UserId";
  client_properties.registrar_vk = "RegistrarVk";
  client_properties.pre_flight = "PreFlight";
  client_properties.fee_amount = std::numeric_limits<double>::max();
  client_properties.user_changed_fee = true;
  client_properties.auto_contribute = true;
  client_properties.rewards_enabled = true;
  client_properties.inline_tips = GetInlineTips(9);

  const std::string json = "{\"walletInfo\":{\"paymentId\":\"PaymentId\",\"addressCARD_ID\":\"AddressCardId\",\"keyInfoSeed\":\"/kBv0C7wS4EBY3EIa780pYLrhryP3IWCfElIehufOFw=\"},\"bootStamp\":4294967295,\"reconcileStamp\":4294967295,\"personaId\":\"PersonaId\",\"userId\":\"UserId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"preFlight\":\"PreFlight\",\"fee_currency\":\"FeeCurrency\",\"settings\":\"Settings\",\"fee_amount\":1.7976931348623157e308,\"user_changed_fee\":true,\"days\":4294967295,\"rewards_enabled\":true,\"auto_contribute\":true,\"transactions\":[{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"contribution_probi\":\"ContributionProbi\",\"submissionStamp\":\"SubmissionTimestamp\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"surveyorIds\":[\"SurveyorId\"],\"votes\":4294967295,\"ballots\":[{\"publisher\":\"Publisher\",\"offset\":4294967295}]},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"contribution_probi\":\"ContributionProbi\",\"submissionStamp\":\"SubmissionTimestamp\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"surveyorIds\":[\"SurveyorId\"],\"votes\":4294967295,\"ballots\":[{\"publisher\":\"Publisher\",\"offset\":4294967295}]},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"contribution_probi\":\"ContributionProbi\",\"submissionStamp\":\"SubmissionTimestamp\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"masterUserToken\":\"MasterUserToken\",\"surveyorIds\":[\"SurveyorId\"],\"votes\":4294967295,\"ballots\":[{\"publisher\":\"Publisher\",\"offset\":4294967295}]}],\"ballots\":[{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"},{\"viewingId\":\"ViewingId\",\"surveyorId\":\"SurveyorId\",\"publisher\":\"Publisher\",\"offset\":4294967295,\"prepareBallot\":\"PrepareBallot\"}],\"batch\":[{\"publisher\":\"Publisher\",\"batchVotesInfo\":[{\"surveyorId\":\"SurveyorId\",\"proof\":\"Proof\"}]}],\"current_reconciles\":{\"0\":{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"},\"1\":{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"},\"2\":{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"},\"3\":{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"},\"4\":{\"viewingId\":\"ViewingId\",\"anonizeViewingId\":\"AnonizeViewingId\",\"registrarVK\":\"RegistrarVk\",\"preFlight\":\"PreFlight\",\"masterUserToken\":\"MasterUserToken\",\"surveyorInfo\":{\"surveyorId\":\"SurveyorId\"},\"timestamp\":4294967295,\"amount\":\"Amount\",\"currency\":\"Currency\",\"fee\":1.7976931348623157e308,\"type\":8,\"rates\":{\"BAT\":1.0,\"BTC\":4.0,\"ETH\":2.0,\"EUR\":6.0,\"LTC\":3.0,\"USD\":5.0},\"directions\":[{\"amount_percent\":1.7976931348623157e308,\"publisher_key\":\"PublisherKey\"}],\"retry_step\":1,\"retry_level\":2147483647,\"destination\":\"Destination\",\"proof\":\"Proof\"}},\"walletProperties\":{\"fee_amount\":1.7976931348623157e308,\"parameters\":{\"adFree\":{\"fee\":{\"BAT\":1.7976931348623157e308},\"choices\":{\"BAT\":[5.0,10.0,15.0,20.0,25.0,50.0,100.0]}}}},\"inlineTip\":{\"0\":true,\"1\":true,\"2\":true,\"3\":true,\"4\":true,\"5\":true,\"6\":true,\"7\":true,\"8\":true}}";  // NOLINT

  // Act
  ClientProperties expected_client_properties;
  const ClientState client_state;
  client_state.FromJson(json, &expected_client_properties);

  // Assert
  EXPECT_EQ(expected_client_properties, client_properties);
}

}  // namespace ledger
