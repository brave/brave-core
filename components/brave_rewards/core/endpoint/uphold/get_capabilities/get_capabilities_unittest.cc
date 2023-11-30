/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"

#include <map>
#include <optional>
#include <string>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/uphold/uphold_capabilities.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetCapabilitiesTest.*

namespace brave_rewards::internal {

using ::testing::_;
using uphold::Capabilities;

namespace endpoint {
namespace uphold {

class GetCapabilitiesTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  GetCapabilities get_capabilities_{mock_engine_impl_};
};

TEST_F(GetCapabilitiesTest, ServerReturns200OKSufficientReceivesAndSends) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, true);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceives1) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, true);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceives2) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, true);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientSends1) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, false);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientSends2) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, true);
        EXPECT_EQ(capabilities.can_send, false);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceivesAndSends1) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, false);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns200OKInsufficientReceivesAndSends2) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_OK;
        response->body = R"(
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(capabilities.can_receive, false);
        EXPECT_EQ(capabilities.can_send, false);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturns401Unauthorized) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_UNAUTHORIZED;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
        EXPECT_EQ(capabilities.can_receive, std::nullopt);
        EXPECT_EQ(capabilities.can_send, std::nullopt);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetCapabilitiesTest, ServerReturnsUnexpectedHTTPStatus) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = net::HTTP_INTERNAL_SERVER_ERROR;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetCapabilitiesCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, Capabilities capabilities) {
        EXPECT_EQ(result, mojom::Result::FAILED);
        EXPECT_EQ(capabilities.can_receive, std::nullopt);
        EXPECT_EQ(capabilities.can_send, std::nullopt);
      });
  get_capabilities_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                            callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal
