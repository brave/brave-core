/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/uphold/uphold_capabilities.h"
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
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  GetCapabilities get_capabilities_{&mock_ledger_impl_};
};

TEST_F(GetCapabilitiesTest, ServerReturns200OKSufficientReceivesAndSends) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, true);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceives1) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, true);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceives2) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, true);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientSends1) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientSends2) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceivesAndSends1) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceivesAndSends2) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
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

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::LEDGER_OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, false);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturns401Unauthorized) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_UNAUTHORIZED;
            std::move(callback).Run(std::move(response));
          }));

  get_capabilities_.Request(
      "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
      base::BindOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(capabilities.can_receive, absl::nullopt);
        EXPECT_EQ(capabilities.can_send, absl::nullopt);
      }));
}

TEST_F(GetCapabilitiesTest, ServerReturnsUnexpectedHTTPStatus) {
  ON_CALL(*mock_ledger_impl_.ledger_client(), LoadURL(_, _))
      .WillByDefault(
          Invoke([](mojom::UrlRequestPtr, client::LoadURLCallback callback) {
            mojom::UrlResponse response;
            response.status_code = net::HTTP_INTERNAL_SERVER_ERROR;
            std::move(callback).Run(response);
          }));

  get_capabilities_.Request(
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
