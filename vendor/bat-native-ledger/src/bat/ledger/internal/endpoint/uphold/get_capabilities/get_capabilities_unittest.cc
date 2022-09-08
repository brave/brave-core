/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoint/uphold/get_capabilities/get_capabilities.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/uphold/uphold_capabilities.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetCapabilitiesTest.*

using ledger::uphold::Capabilities;
using ::testing::_;
using ::testing::Invoke;

namespace ledger {
namespace endpoint {
namespace uphold {

class GetCapabilitiesTest : public testing::Test {
 protected:
  GetCapabilitiesTest()
      : mock_ledger_client_(std::make_unique<ledger::MockLedgerClient>()),
        mock_ledger_impl_(std::make_unique<ledger::MockLedgerImpl>(
            mock_ledger_client_.get())),
        get_capabilities_(
            std::make_unique<GetCapabilities>(mock_ledger_impl_.get())) {}

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetCapabilities> get_capabilities_;
};

TEST_F(GetCapabilitiesTest, ServerReturns200OKSufficientReceivesAndSends) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": true,
    "key": "receives",
    "name": "Receives",
    "requirements": [],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": true,
    "key": "sends",
    "name": "Sends",
    "requirements": [],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, true);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceives1) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": true,
    "key": "receives",
    "name": "Receives",
    "requirements": [
      "user-must-submit-customer-due-diligence"
    ],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": true,
    "key": "sends",
    "name": "Sends",
    "requirements": [],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, true);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceives2) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": false,
    "key": "receives",
    "name": "Receives",
    "requirements": [],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": true,
    "key": "sends",
    "name": "Sends",
    "requirements": [],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(std::move(response));
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, true);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientSends1) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": true,
    "key": "receives",
    "name": "Receives",
    "requirements": [],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": true,
    "key": "sends",
    "name": "Sends",
    "requirements": [
      "user-must-submit-customer-due-diligence"
    ],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientSends2) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": true,
    "key": "receives",
    "name": "Receives",
    "requirements": [],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": false,
    "key": "sends",
    "name": "Sends",
    "requirements": [],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceivesAndSends1) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": true,
    "key": "receives",
    "name": "Receives",
    "requirements": [
      "user-must-submit-customer-due-diligence"
    ],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": true,
    "key": "sends",
    "name": "Sends",
    "requirements": [
      "user-must-submit-customer-due-diligence"
    ],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceivesAndSends2) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_OK;
            response.body = R"(
[
  {
    "category": "permissions",
    "enabled": false,
    "key": "receives",
    "name": "Receives",
    "requirements": [],
    "restrictions": []
  },
  {
    "category": "permissions",
    "enabled": false,
    "key": "sends",
    "name": "Sends",
    "requirements": [],
    "restrictions": []
  }
]
            )";
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns401Unauthorized) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            std::move(callback).Run(std::move(response));
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(capabilities.can_receive, absl::nullopt);
        EXPECT_EQ(capabilities.can_send, absl::nullopt);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturnsUnexpectedHTTPStatus) {
  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_INTERNAL_SERVER_ERROR;
            std::move(callback).Run(response);
          }));

  get_capabilities_->Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
        EXPECT_EQ(capabilities.can_receive, absl::nullopt);
        EXPECT_EQ(capabilities.can_send, absl::nullopt);
      }));
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
