/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_client_mock.h"

#include <utility>

#include "brave/components/brave_rewards/core/test/test_ledger_client.h"

using ::testing::_;

namespace ledger {

MockLedgerClient::MockLedgerClient() {
  // If a mock function takes a response callback as its parameter,
  // its default action won't run the callback passed to it, which is an error
  // if the callback's corresponding interface pipe is not closed. Make sure we
  // don't drop response callbacks by explicitly specifying default actions for
  // such functions.
  ON_CALL(*this, LoadLedgerState(_)).WillByDefault([](auto callback) {
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
  });

  ON_CALL(*this, LoadPublisherState(_)).WillByDefault([](auto callback) {
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
  });

  ON_CALL(*this, FetchFavIcon(_, _, _))
      .WillByDefault([](const std::string&, const std::string&, auto callback) {
        std::move(callback).Run(false, "");
      });

  ON_CALL(*this, LoadURL(_, _))
      .WillByDefault([](mojom::UrlRequestPtr, auto callback) {
        std::move(callback).Run(nullptr);
      });

  ON_CALL(*this, GetBooleanState(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(*this, SetBooleanState(_, _, _))
      .WillByDefault([](const std::string&, bool, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetIntegerState(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0);
      });

  ON_CALL(*this, SetIntegerState(_, _, _))
      .WillByDefault([](const std::string&, int32_t, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetDoubleState(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0.0);
      });

  ON_CALL(*this, SetDoubleState(_, _, _))
      .WillByDefault([](const std::string&, double, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetStringState(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run("");
      });

  ON_CALL(*this, SetStringState(_, _, _))
      .WillByDefault([](const std::string&, const std::string&, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetInt64State(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0);
      });

  ON_CALL(*this, SetInt64State(_, _, _))
      .WillByDefault([](const std::string&, int64_t, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetUint64State(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0);
      });

  ON_CALL(*this, SetUint64State(_, _, _))
      .WillByDefault([](const std::string&, uint64_t, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetValueState(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(base::Value());
      });

  ON_CALL(*this, SetValueState(_, _, _))
      .WillByDefault([](const std::string&, base::Value, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, GetTimeState(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(base::Time());
      });

  ON_CALL(*this, SetTimeState(_, _, _))
      .WillByDefault([](const std::string&, base::Time, auto callback) {
        std::move(callback).Run();
      });

  ON_CALL(*this, ClearState(_, _))
      .WillByDefault(
          [](const std::string&, auto callback) { std::move(callback).Run(); });

  ON_CALL(*this, GetBooleanOption(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(false);
      });

  ON_CALL(*this, GetIntegerOption(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0);
      });

  ON_CALL(*this, GetDoubleOption(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0.0);
      });

  ON_CALL(*this, GetStringOption(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run("");
      });

  ON_CALL(*this, GetInt64Option(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0);
      });

  ON_CALL(*this, GetUint64Option(_, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run(0);
      });

  ON_CALL(*this, GetLegacyWallet(_)).WillByDefault([](auto callback) {
    std::move(callback).Run("");
  });

  ON_CALL(*this, ShowNotification(_, _, _))
      .WillByDefault([](const std::string&, const std::vector<std::string>&,
                        auto callback) {
        std::move(callback).Run(mojom::Result::LEDGER_ERROR);
      });

  ON_CALL(*this, GetClientInfo(_)).WillByDefault([](auto callback) {
    std::move(callback).Run(nullptr);
  });

  ON_CALL(*this, RunDBTransaction(_, _))
      .WillByDefault([](mojom::DBTransactionPtr, auto callback) {
        std::move(callback).Run(nullptr);
      });

  ON_CALL(*this, GetCreateScript(_)).WillByDefault([](auto callback) {
    std::move(callback).Run("", 0);
  });

  ON_CALL(*this, DeleteLog(_)).WillByDefault([](auto callback) {
    std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  });
}

MockLedgerClient::~MockLedgerClient() = default;

void MockLedgerClient::EncryptString(const std::string& value,
                                     EncryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::EncryptString(value));
}

void MockLedgerClient::DecryptString(const std::string& value,
                                     DecryptStringCallback callback) {
  std::move(callback).Run(FakeEncryption::DecryptString(value));
}

}  // namespace ledger
