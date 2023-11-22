/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"

#include <memory>
#include <utility>

#include "base/functional/callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"

namespace ai_chat {

class RemoteCompletionClientUnitTest : public testing::Test {
 public:
  RemoteCompletionClientUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    ai_chat_api_ =
        std::make_unique<RemoteCompletionClient>(shared_url_loader_factory_);
  }

  ~RemoteCompletionClientUnitTest() override = default;

  void SendMesage(RemoteCompletionClient::ResponseCallback callback,
                  const std::string& text) {
    ai_chat_api_->set_response_callback_for_testing(callback);
    ai_chat_api_->SendDataForTesting(text);
  }

 protected:
  std::unique_ptr<RemoteCompletionClient> ai_chat_api_ = nullptr;

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(RemoteCompletionClientUnitTest, ParseJson) {
  base::RunLoop run_loop;
  SendMesage(base::BindRepeating(
                 [](base::RunLoop* run_loop, const std::string& response) {
                   EXPECT_EQ(" Hello there!", response);
                   run_loop->Quit();
                 },
                 &run_loop),
             "data: {\"completion\": \" Hello there!\", \"stop\": null}");
  run_loop.Run();

  base::RunLoop run_loop2;
  SendMesage(
      base::BindRepeating(
          [](base::RunLoop* run_loop, const std::string& response) {
            EXPECT_EQ(" Hello there! How are you?", response);
            run_loop->Quit();
          },
          &run_loop2),
      "data: {\"completion\": \" Hello there! How are you?\", \"stop\": null}");
  run_loop2.Run();

  // This test verifies that the callback is not called when the response is
  // "[DONE]". We use a run loop to wait for the callback to be called, and we
  // expect it to never be called. Therefore, we use RunUntilIdle() instead of
  // Run(), since Run() would time out waiting for the callback to be called.
  base::RunLoop run_loop3;
  SendMesage(
      base::BindRepeating([](base::RunLoop* run_loop,
                             const std::string& response) { run_loop->Quit(); },
                          &run_loop3),
      "data: [DONE]");
  run_loop3.RunUntilIdle();
}

}  // namespace ai_chat
