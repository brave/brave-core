/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/unstoppable_domains_multichain_calls.h"

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet::unstoppable_domains {

class MultichainCallsUnitTest : public testing::Test {
 public:
  using CallbackType = MultichainCalls<std::string, std::string>::CallbackType;

  std::string domain() const { return "brave.com"; }

  MultichainCalls<std::string, std::string>& chain_calls() {
    return chain_calls_;
  }

 private:
  MultichainCalls<std::string, std::string> chain_calls_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(MultichainCallsUnitTest, ManyCallbacks) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  base::MockCallback<CallbackType> cb2;
  EXPECT_CALL(cb1, Run(_, _, _)).Times(0);
  EXPECT_CALL(cb2, Run(_, _, _)).Times(0);

  chain_calls().AddCallback(domain(), cb1.Get());
  EXPECT_TRUE(chain_calls().HasCall(domain()));
  chain_calls().AddCallback(domain(), cb2.Get());

  chain_calls().SetResult(domain(), mojom::kPolygonMainnetChainId, "polygon");
  chain_calls().SetResult(domain(), mojom::kBaseMainnetChainId, "base");
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&cb1);
  testing::Mock::VerifyAndClearExpectations(&cb2);

  EXPECT_CALL(cb1, Run("polygon", mojom::ProviderError::kSuccess, ""));
  EXPECT_CALL(cb2, Run("polygon", mojom::ProviderError::kSuccess, ""));
  chain_calls().SetResult(domain(), mojom::kMainnetChainId, "mainnet");
  chain_calls().SetResult(domain(), mojom::kBaseMainnetChainId, "base");
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, PolygonResult) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("polygon", mojom::ProviderError::kSuccess, ""));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetResult(domain(), mojom::kPolygonMainnetChainId, "polygon");
  chain_calls().SetResult(domain(), mojom::kBaseMainnetChainId, "base");
  chain_calls().SetResult(domain(), mojom::kMainnetChainId, "mainnet");

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, BaseError) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("", mojom::ProviderError::kInternalError, "some error"));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetError(domain(), mojom::kBaseMainnetChainId,
                         mojom::ProviderError::kInternalError, "some error");
  chain_calls().SetNoResult(domain(), mojom::kPolygonMainnetChainId);
  chain_calls().SetResult(domain(), mojom::kMainnetChainId, "mainnet");

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, BaseResult) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("base", mojom::ProviderError::kSuccess, ""));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetNoResult(domain(), mojom::kPolygonMainnetChainId);
  chain_calls().SetResult(domain(), mojom::kBaseMainnetChainId, "base");
  chain_calls().SetResult(domain(), mojom::kMainnetChainId, "mainnet");

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, PolygonError) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("", mojom::ProviderError::kInternalError, "some error"));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetError(domain(), mojom::kPolygonMainnetChainId,
                         mojom::ProviderError::kInternalError, "some error");
  chain_calls().SetResult(domain(), mojom::kBaseMainnetChainId, "base");
  chain_calls().SetResult(domain(), mojom::kMainnetChainId, "mainnet");

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, MainnetResult) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("mainnet", mojom::ProviderError::kSuccess, ""));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetNoResult(domain(), mojom::kPolygonMainnetChainId);
  chain_calls().SetNoResult(domain(), mojom::kBaseMainnetChainId);
  chain_calls().SetResult(domain(), mojom::kMainnetChainId, "mainnet");

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, MainnetError) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("", mojom::ProviderError::kInternalError, "some error"));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetNoResult(domain(), mojom::kPolygonMainnetChainId);
  chain_calls().SetNoResult(domain(), mojom::kBaseMainnetChainId);
  chain_calls().SetError(domain(), mojom::kMainnetChainId,
                         mojom::ProviderError::kInternalError, "some error");

  base::RunLoop().RunUntilIdle();
}

TEST_F(MultichainCallsUnitTest, NoResult) {
  EXPECT_FALSE(chain_calls().HasCall(domain()));

  base::MockCallback<CallbackType> cb1;
  EXPECT_CALL(cb1, Run("", mojom::ProviderError::kSuccess, ""));

  chain_calls().AddCallback(domain(), cb1.Get());

  chain_calls().SetNoResult(domain(), mojom::kPolygonMainnetChainId);
  chain_calls().SetNoResult(domain(), mojom::kBaseMainnetChainId);
  chain_calls().SetNoResult(domain(), mojom::kMainnetChainId);

  base::RunLoop().RunUntilIdle();
}

}  // namespace brave_wallet::unstoppable_domains
