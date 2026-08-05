// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "brave/browser/ai_chat/model_service_factory.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/remote_models_serialization.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_status_code.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

mojom::ModelPtr MakeServerTestModel(const std::string& key) {
  auto leo_opts = mojom::LeoModelOptions::New();
  leo_opts->name = key + "-model";
  leo_opts->display_maker = "Test Corp";
  leo_opts->description = "A test model";
  leo_opts->category = mojom::ModelCategory::CHAT;
  leo_opts->access = mojom::ModelAccess::BASIC;
  leo_opts->max_associated_content_length = 100000;
  leo_opts->long_conversation_warning_character_limit = 200000;

  auto model = mojom::Model::New();
  model->key = key;
  model->display_name = key + " Display";
  model->is_suggested_model = false;
  model->is_near_model = false;
  model->supported_capabilities = {mojom::ConversationCapability::CHAT};
  model->options = mojom::ModelOptions::NewLeoModelOptions(std::move(leo_opts));
  return model;
}

}  // namespace

// End-to-end coverage of the remote-models feature over a real network stack:
// an embedded HTTPS server stands in for the remote-models endpoint, and
// `kAIChatRemoteModelsConfig` is enabled with a short TTL so refresh-timer
// behavior is observable without mocking the timer itself.
class RemoteModelsIntegrationBrowserTest : public InProcessBrowserTest {
 public:
  RemoteModelsIntegrationBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kAIChatRemoteModelsConfig, {{"cache_ttl", "2s"}});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.RegisterRequestHandler(base::BindRepeating(
        &RemoteModelsIntegrationBrowserTest::HandleModelsRequest,
        base::Unretained(this)));
    https_server_.StartAcceptingConnections();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    ASSERT_TRUE(https_server_.InitializeAndListen());
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_.host_port_pair().ToString() +
            ",EXCLUDE localhost");
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  // Sets the models the server responds with, or `std::nullopt` to make it
  // respond with an HTTP error. Thread-safe: the embedded server dispatches
  // request handlers on its own IO thread.
  void SetServerModels(std::optional<std::vector<mojom::ModelPtr>> models) {
    base::AutoLock lock(lock_);
    server_models_ = std::move(models);
  }

  int GetRequestCount() {
    base::AutoLock lock(lock_);
    return request_count_;
  }

  ModelService* GetModelService() {
    return ai_chat::ModelServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  void ShowSidePanelEntry(SidePanelEntryId id) {
    browser()->GetFeatures().side_panel_ui()->Show(id);
  }

 private:
  std::unique_ptr<net::test_server::HttpResponse> HandleModelsRequest(
      const net::test_server::HttpRequest& request) {
    if (request.GetURL().path() != "/v1/models") {
      return nullptr;
    }

    base::AutoLock lock(lock_);
    ++request_count_;

    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    if (!server_models_) {
      response->set_code(net::HTTP_INTERNAL_SERVER_ERROR);
      return response;
    }
    response->set_code(net::HTTP_OK);
    response->set_content_type("application/json");
    response->set_content(
        base::WriteJson(SerializeModels(*server_models_)).value_or(""));
    return response;
  }

  net::test_server::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  base::test::ScopedFeatureList scoped_feature_list_;

  base::Lock lock_;
  int request_count_ GUARDED_BY(lock_) = 0;
  std::optional<std::vector<mojom::ModelPtr>> server_models_ GUARDED_BY(lock_);
};

IN_PROC_BROWSER_TEST_F(RemoteModelsIntegrationBrowserTest,
                       NoFetchWhenLeoNeverOpened) {
  std::vector<mojom::ModelPtr> models;
  models.push_back(MakeServerTestModel("remote-test-model"));
  SetServerModels(std::move(models));

  // Give the browser a moment to have started everything it's going to.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(1));
  run_loop.Run();

  EXPECT_EQ(GetRequestCount(), 0);
}

IN_PROC_BROWSER_TEST_F(RemoteModelsIntegrationBrowserTest,
                       OpeningSidePanelFetchesRemoteModelsAndUpdatesList) {
  std::vector<mojom::ModelPtr> models;
  models.push_back(MakeServerTestModel("remote-test-model"));
  SetServerModels(std::move(models));

  ShowSidePanelEntry(SidePanelEntryId::kChatUI);

  ASSERT_TRUE(base::test::RunUntil(
      [&] { return GetModelService()->GetModel("remote-test-model"); }));
  EXPECT_EQ(GetRequestCount(), 1);
}

IN_PROC_BROWSER_TEST_F(RemoteModelsIntegrationBrowserTest,
                       FetchFailureFallsBackToHardcodedModels) {
  SetServerModels(std::nullopt);

  ShowSidePanelEntry(SidePanelEntryId::kChatUI);

  ASSERT_TRUE(base::test::RunUntil([&] { return GetRequestCount() >= 1; }));
  EXPECT_TRUE(GetModelService()->GetModel(kChatAutomaticModelKey));
}

IN_PROC_BROWSER_TEST_F(RemoteModelsIntegrationBrowserTest,
                       SwitchingAwayStopsFurtherRefreshes) {
  std::vector<mojom::ModelPtr> models;
  models.push_back(MakeServerTestModel("remote-test-model"));
  SetServerModels(std::move(models));

  ShowSidePanelEntry(SidePanelEntryId::kChatUI);
  ASSERT_TRUE(base::test::RunUntil([&] { return GetRequestCount() >= 1; }));

  ShowSidePanelEntry(SidePanelEntryId::kBookmarks);

  // The 2s TTL would have fired at least one more refresh by now if the
  // timer were still running.
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Seconds(5));
  run_loop.Run();

  EXPECT_EQ(GetRequestCount(), 1);
}

IN_PROC_BROWSER_TEST_F(RemoteModelsIntegrationBrowserTest,
                       SwitchingBackWithinTTLDoesNotRefetch) {
  std::vector<mojom::ModelPtr> models;
  models.push_back(MakeServerTestModel("remote-test-model"));
  SetServerModels(std::move(models));

  ShowSidePanelEntry(SidePanelEntryId::kChatUI);
  ASSERT_TRUE(base::test::RunUntil([&] { return GetRequestCount() >= 1; }));

  ShowSidePanelEntry(SidePanelEntryId::kBookmarks);
  ShowSidePanelEntry(SidePanelEntryId::kChatUI);

  ASSERT_TRUE(base::test::RunUntil([&] {
    return GetModelService()->GetRemoteModelsVisibleSurfaceCountForTesting() ==
           1u;
  }));
  EXPECT_EQ(GetRequestCount(), 1);
}

}  // namespace ai_chat
