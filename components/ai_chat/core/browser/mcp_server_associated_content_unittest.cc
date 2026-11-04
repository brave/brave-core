// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/mcp_server_associated_content.h"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "base/check_deref.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_host.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_test_server.h"
#include "brave/components/ai_chat/core/browser/mcp/test/deferred_os_crypt_async.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider_factory.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "build/build_config.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::UnorderedElementsAre;

constexpr char kInitializeResult[] =
    R"({"protocolVersion":"2025-11-25","capabilities":{"tools":{}},
        "serverInfo":{"name":"test","version":"1"}})";

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  void GetPremiumStatus(
      mojom::Service::GetPremiumStatusCallback callback) override {
    std::move(callback).Run(mojom::PremiumStatus::Inactive,
                            mojom::PremiumInfo::New());
  }
};

// Stands in for an open conversation UI, which keeps the conversation loaded,
// and records the tool lists the browser pushes to it.
class TestConversationUI : public mojom::ConversationUI {
 public:
  explicit TestConversationUI(ConversationHandler& handler) {
    handler.Bind(receiver_.BindNewPipeAndPassRemote());
  }
  ~TestConversationUI() override = default;

  // Waits for the browser to push that the content with `content_uuid` offers
  // the tools named `names`.
  void WaitForPushedTools(std::string_view content_uuid,
                          const std::vector<std::string>& names) {
    const PushedTools expected(content_uuid, names);
    while (!std::ranges::contains(pushed_tools_, expected)) {
      base::RunLoop run_loop;
      quit_ = run_loop.QuitClosure();
      run_loop.Run();
    }
  }

  // Whether the browser has pushed that the content with `content_uuid` offers
  // the tools named `names`, since the pushes were last cleared.
  bool WasPushed(std::string_view content_uuid,
                 const std::vector<std::string>& names) {
    receiver_.FlushForTesting();
    return std::ranges::contains(pushed_tools_,
                                 PushedTools(content_uuid, names));
  }
  void ClearPushedTools() { pushed_tools_.clear(); }

  // mojom::ConversationUI:
  void OnContentToolsChanged(const std::string& content_uuid,
                             std::vector<mojom::ToolInfoPtr> tools) override {
    std::vector<std::string> names;
    for (const mojom::ToolInfoPtr& tool : tools) {
      names.push_back(tool->name);
    }
    pushed_tools_.emplace_back(content_uuid, std::move(names));
    if (quit_) {
      std::move(quit_).Run();
    }
  }
  void OnConversationHistoryUpdate(mojom::ConversationTurnPtr entry) override {}
  void OnAPIRequestInProgress(bool in_progress) override {}
  void OnAPIResponseError(mojom::APIError error,
                          mojom::APIErrorDetailsPtr details) override {}
  void OnTaskStateChanged(mojom::TaskState task_state) override {}
  void OnModelDataChanged(const std::string& conversation_model_key,
                          const std::string& default_model_key,
                          std::vector<mojom::ModelPtr> all_models) override {}
#if BUILDFLAG(IS_IOS)
  void OnSuggestedQuestionsChanged(
      const std::vector<std::string>& questions,
      mojom::SuggestionGenerationStatus status) override {}
#endif
  void OnAssociatedContentInfoChanged(
      std::vector<mojom::AssociatedContentPtr> associated_content) override {}
  void OnVisionModelSwitchChanged(
      const std::optional<std::string>& model_key) override {}
  void OnContentAgentChanged(bool is_content_agent_chosen,
                             const std::vector<mojom::ConversationCapability>&
                                 capabilities_enabled) override {}
  void OnConversationDeleted() override {}

 private:
  using PushedTools = std::pair<std::string, std::vector<std::string>>;

  std::vector<PushedTools> pushed_tools_;
  base::OnceClosure quit_;
  mojo::Receiver<mojom::ConversationUI> receiver_{this};
};

// A `tools/list` result with a tool for each of `names`, each described as
// `description` unless it's empty.
std::string ToolsListResult(const std::vector<std::string>& names,
                            std::string_view description = "") {
  base::ListValue tools;
  for (const std::string& name : names) {
    base::DictValue tool =
        base::DictValue()
            .Set("name", name)
            .Set("inputSchema", base::DictValue().Set("type", "object"));
    if (!description.empty()) {
      tool.Set("description", description);
    }
    tools.Append(std::move(tool));
  }
  return *base::WriteJson(base::DictValue().Set("tools", std::move(tools)));
}

std::vector<std::string> NumberedToolNames(std::string_view prefix,
                                           size_t count) {
  std::vector<std::string> names;
  for (size_t i = 0; i < count; ++i) {
    names.push_back(base::StrCat({prefix, "_", base::NumberToString(i)}));
  }
  return names;
}

mojom::ConversationTurnPtr HumanTurn(std::string uuid) {
  return mojom::ConversationTurn::New(
      std::move(uuid), std::nullopt /* thread_uuid */,
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "Question",
      std::nullopt, std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
      std::nullopt, nullptr /* skill */, false, std::nullopt, nullptr,
      std::vector<std::string>{} /* child_thread_uuids */);
}

mojom::ConversationTurnPtr AssistantTurn(
    std::string uuid,
    std::vector<mojom::ConversationEntryEventPtr> events) {
  return mojom::ConversationTurn::New(
      std::move(uuid), std::nullopt /* thread_uuid */,
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, "Answer",
      std::nullopt, std::nullopt, std::move(events), base::Time::Now(),
      std::nullopt, std::nullopt, nullptr /* skill */, false, std::nullopt,
      nullptr, std::vector<std::string>{} /* child_thread_uuids */);
}

void WaitUntilReady(MCPHost& host) {
  base::RunLoop run_loop;
  host.RunWhenReady(run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace

class MCPServerAssociatedContentTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());
#if BUILDFLAG(IS_IOS)
    prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, true);
#endif

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    model_service_ = std::make_unique<ModelService>(
        &prefs_, os_crypt_.get(), network::NetworkContextGetter(),
        /*url_loader_factory=*/nullptr, base::FilePath());
    // Several servers' requests interleave, so each is answered once it's
    // sent, rather than waited for in turn.
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &MCPServerAssociatedContentTest::OnRequest, base::Unretained(this)));
    CreateService();
    WaitUntilReady(host());
  }

  void TearDown() override {
    ai_chat_service_.reset();
    // Each service's database is written and closed on its own sequence,
    // which must be done with the directory before it's deleted.
    base::ThreadPoolInstance::Get()->FlushForTesting();
  }

 protected:
  void CreateService() { CreateService(*os_crypt_); }
  void CreateService(os_crypt_async::OSCryptAsync& os_crypt) {
    // Each service has its own storage, so one isn't opened while another
    // closes.
    const base::FilePath profile_path = temp_directory_.GetPath().AppendASCII(
        base::NumberToString(++service_count_));
    ASSERT_TRUE(base::CreateDirectory(profile_path));
    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), /*tab_tracker_service=*/nullptr,
        std::make_unique<MockAIChatCredentialManager>(base::NullCallback(),
                                                      &local_state_),
        &prefs_, /*ai_chat_metrics=*/nullptr, &os_crypt,
        url_loader_factory_.GetSafeWeakWrapper(), "", profile_path);
  }

  void ResetService() { ResetService(*os_crypt_); }
  void ResetService(os_crypt_async::OSCryptAsync& os_crypt) {
    ai_chat_service_.reset();
    CreateService(os_crypt);
  }

  // The service has one, as the AIChatMCP feature is enabled.
  MCPHost& host() {
    return CHECK_DEREF(ai_chat_service_->GetMCPHostForTesting());
  }

  // Adds a server that offers a tool for each of `tool_names`.
  mojom::MCPServerPtr AddServer(std::string_view name,
                                std::string_view endpoint,
                                std::vector<std::string> tool_names,
                                bool enabled = true,
                                bool auto_attach = false) {
    tools_[GURL(endpoint).spec()] = ToolsListResult(tool_names);
    base::test::TestFuture<
        base::expected<mojom::MCPServerPtr, mojom::MCPServerError>>
        future;
    host().AddServer(
        mojom::MCPServerParams::New(std::string(name), GURL(endpoint), enabled,
                                    auto_attach),
        std::nullopt, future.GetCallback());
    auto result = future.Take();
    EXPECT_TRUE(result.has_value());
    if (!result.has_value()) {
      return nullptr;
    }
    EXPECT_EQ(enabled, !!host().GetClient((*result)->id));
    return std::move(*result);
  }

  // Keeps the server's token unless it's given another.
  void UpdateServer(const mojom::MCPServer& server,
                    std::string_view name,
                    std::string_view endpoint,
                    bool enabled,
                    const std::optional<std::string>& token = std::nullopt) {
    base::test::TestFuture<
        base::expected<mojom::MCPServerPtr, mojom::MCPServerError>>
        future;
    host().UpdateServer(
        server.id,
        mojom::MCPServerParams::New(std::string(name), GURL(endpoint), enabled,
                                    server.auto_attach),
        token, future.GetCallback());
    EXPECT_TRUE(future.Take().has_value());
  }

  // Requests to `endpoint` aren't answered until it's released.
  void Hold(std::string_view endpoint) {
    held_endpoints_.insert(GURL(endpoint).spec());
  }
  void Release(std::string_view endpoint) {
    held_endpoints_.erase(GURL(endpoint).spec());
    AnswerRequests();
  }

  void OnRequest(const network::ResourceRequest& request) {
    // Answered once it's among the pending requests.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&MCPServerAssociatedContentTest::AnswerRequests,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void AnswerRequests() {
    for (size_t i = 0; i < mcp_server_.RequestCount(); ++i) {
      const GURL url = mcp_server_.Request(i).url;
      if (answered_.contains(i) || held_endpoints_.contains(url.spec())) {
        continue;
      }
      answered_.insert(i);
      const std::string method = mcp_server_.Method(i);
      if (method == "initialize") {
        mcp_server_.RespondResult(i, kInitializeResult);
      } else if (method == "tools/list" &&
                 failing_endpoints_.contains(url.spec())) {
        mcp_server_.RespondError(i, -32603, "Internal error");
      } else if (method == "tools/list") {
        mcp_server_.RespondResult(i, tools_[url.spec()]);
      } else if (method == "tools/call") {
        mcp_server_.RespondResult(
            i, R"({"content":[{"type":"text","text":"Done"}]})");
      } else {
        // Notifications, and the requests that end sessions.
        mcp_server_.RespondAccepted(i);
      }
    }
  }

  AssociatedContentManager& Manager(ConversationHandler* conversation) {
    return *conversation->associated_content_manager();
  }

  // The MCP servers attached to `conversation`, as the UI is told of them.
  std::vector<mojom::AssociatedContentPtr> GetMCPServers(
      ConversationHandler* conversation) {
    std::vector<mojom::AssociatedContentPtr> servers;
    for (mojom::AssociatedContentPtr& content :
         Manager(conversation).GetAssociatedContent()) {
      if (content->content_type == mojom::ContentType::MCPServer) {
        servers.push_back(std::move(content));
      }
    }
    return servers;
  }

  std::vector<std::string> GetMCPServerTitles(
      ConversationHandler* conversation) {
    std::vector<std::string> titles;
    for (const mojom::AssociatedContentPtr& server :
         GetMCPServers(conversation)) {
      titles.push_back(server->title);
    }
    return titles;
  }

  // The uuid of the attached MCP server titled `title`.
  std::string GetMCPServerUuid(ConversationHandler* conversation,
                               std::string_view title) {
    for (const mojom::AssociatedContentPtr& server :
         GetMCPServers(conversation)) {
      if (server->title == title) {
        return server->uuid;
      }
    }
    ADD_FAILURE() << "No MCP server titled " << title;
    return "";
  }

  void Attach(ConversationHandler* conversation,
              const mojom::MCPServer& server) {
    ai_chat_service_->AssociateMCPServer(server.id,
                                         conversation->get_conversation_uuid());
  }

  void Detach(ConversationHandler* conversation, std::string_view title) {
    for (const mojom::AssociatedContentPtr& server :
         GetMCPServers(conversation)) {
      if (server->title == title) {
        ai_chat_service_->DisassociateContent(
            server, conversation->get_conversation_uuid());
        return;
      }
    }
    ADD_FAILURE() << "No MCP server titled " << title;
  }

  std::vector<mojom::ToolInfoPtr> GetToolInfos(
      ConversationHandler* conversation,
      const std::string& content_uuid) {
    base::test::TestFuture<std::vector<mojom::ToolInfoPtr>> future;
    Manager(conversation).GetToolInfos(content_uuid, future.GetCallback());
    return future.Take();
  }

  // Starts a generation loop and returns the display names of its tools.
  std::vector<std::string> RunGenerationLoop(
      ConversationHandler* conversation) {
    base::test::TestFuture<void> done;
    Manager(conversation).UpdateToolsForNewGenerationLoop(done.GetCallback());
    EXPECT_TRUE(done.Wait());
    return GetLoopToolNames(conversation);
  }

  std::vector<std::string> GetLoopToolNames(ConversationHandler* conversation) {
    std::vector<std::string> names;
    for (const base::WeakPtr<Tool>& tool : Manager(conversation).GetTools()) {
      names.emplace_back(tool->DisplayName());
    }
    return names;
  }

  base::WeakPtr<Tool> GetLoopTool(ConversationHandler* conversation,
                                  std::string_view display_name) {
    for (const base::WeakPtr<Tool>& tool : Manager(conversation).GetTools()) {
      if (tool->DisplayName() == display_name) {
        return tool;
      }
    }
    return nullptr;
  }

  // The loop's tool that `server` lists as `tool_name`. The tools of servers
  // that list the same name are told apart by the server in the name the model
  // calls them by, not by their position, which depends on which server's
  // answer arrived first.
  base::WeakPtr<Tool> GetLoopToolOfServer(ConversationHandler* conversation,
                                          const mojom::MCPServer& server,
                                          std::string_view tool_name) {
    const std::string model_name =
        base::StrCat({"mcp_", server.slug, "_", tool_name});
    for (const base::WeakPtr<Tool>& tool : Manager(conversation).GetTools()) {
      if (tool->Name() == model_name) {
        return tool;
      }
    }
    return nullptr;
  }

  // Gives `conversation` a response that uses the tool the model calls
  // `tool_name`, with a permission prompt, "tool-use", that can be answered
  // with "Allow for this chat".
  void SetPermissionChallenge(ConversationHandler* conversation,
                              std::string_view tool_name) {
    auto challenge = mojom::PermissionChallenge::New();
    challenge->supports_allow_session = true;
    std::vector<mojom::ConversationEntryEventPtr> events;
    events.push_back(
        mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
            std::string(tool_name), "tool-use", "{}", std::nullopt,
            std::nullopt, std::move(challenge), false)));
    std::vector<mojom::ConversationTurnPtr> history;
    history.push_back(HumanTurn("turn"));
    history.push_back(AssistantTurn("response", std::move(events)));
    conversation->SetChatHistoryForTesting(std::move(history));
  }

  base::test::ScopedFeatureList scoped_feature_list_{features::kAIChatMCP};
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_directory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  MCPTestServer mcp_server_{url_loader_factory_};
  // `tools/list` results, by endpoint.
  std::map<std::string, std::string> tools_;
  // Endpoints whose `tools/list` fails.
  std::set<std::string> failing_endpoints_;
  std::set<std::string> held_endpoints_;
  std::set<size_t> answered_;
  int service_count_ = 0;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  base::WeakPtrFactory<MCPServerAssociatedContentTest> weak_ptr_factory_{this};
};

TEST_F(MCPServerAssociatedContentTest, AutoAttachesOnConversationCreation) {
  AddServer("Auto", "https://auto.example.com/mcp", {"search"},
            /*enabled=*/true, /*auto_attach=*/true);
  AddServer("Manual", "https://manual.example.com/mcp", {"search"});
  AddServer("Disabled", "https://disabled.example.com/mcp", {"search"},
            /*enabled=*/false, /*auto_attach=*/true);

  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  std::vector<mojom::AssociatedContentPtr> servers =
      GetMCPServers(conversation);
  ASSERT_EQ(servers.size(), 1u);
  EXPECT_EQ(servers[0]->title, "Auto");
  EXPECT_EQ(servers[0]->url, GURL("https://auto.example.com/"));
  // Chosen for its tools, so they're attached without waiting to see them.
  EXPECT_TRUE(servers[0]->tools_attached);
  EXPECT_FALSE(servers[0]->conversation_turn_uuid);
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
}

TEST_F(MCPServerAssociatedContentTest, NotAvailableWithoutFeature) {
  mojom::MCPServerPtr server =
      AddServer("Auto", "https://auto.example.com/mcp", {"search"},
                /*enabled=*/true, /*auto_attach=*/true);
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(features::kAIChatMCP);
  ResetService();
  ASSERT_FALSE(ai_chat_service_->GetMCPHostForTesting());

  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  EXPECT_TRUE(GetMCPServers(conversation).empty());
  Attach(conversation, *server);
  EXPECT_TRUE(GetMCPServers(conversation).empty());
}

TEST_F(MCPServerAssociatedContentTest, AttachAndDetach) {
  mojom::MCPServerPtr server =
      AddServer("Server", "https://mcp.example.com/mcp", {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  EXPECT_TRUE(GetMCPServers(conversation).empty());
  EXPECT_FALSE(Manager(conversation).HasMCPServer(server->id));

  Attach(conversation, *server);
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Server"));
  EXPECT_TRUE(Manager(conversation).HasMCPServer(server->id));
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));

  Detach(conversation, "Server");
  EXPECT_TRUE(GetMCPServers(conversation).empty());
  EXPECT_FALSE(Manager(conversation).HasMCPServer(server->id));
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());
}

TEST_F(MCPServerAssociatedContentTest, RejectsUnknownDisabledAndDuplicates) {
  mojom::MCPServerPtr enabled =
      AddServer("Enabled", "https://enabled.example.com/mcp", {"search"});
  mojom::MCPServerPtr disabled =
      AddServer("Disabled", "https://disabled.example.com/mcp", {"search"},
                /*enabled=*/false);
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();

  ai_chat_service_->AssociateMCPServer("unknown",
                                       conversation->get_conversation_uuid());
  Attach(conversation, *disabled);
  EXPECT_TRUE(GetMCPServers(conversation).empty());

  Attach(conversation, *enabled);
  Attach(conversation, *enabled);
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Enabled"));

  // An unknown conversation is ignored.
  ai_chat_service_->AssociateMCPServer(enabled->id, "unknown");
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Enabled"));
}

TEST_F(MCPServerAssociatedContentTest, ToolsAttachedToggle) {
  mojom::MCPServerPtr server =
      AddServer("Server", "https://mcp.example.com/mcp", {"search", "fetch"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "fetch"));

  Manager(conversation).SetToolsAttached(uuid, /*tools_attached=*/false);
  EXPECT_FALSE(GetMCPServers(conversation)[0]->tools_attached);
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());

  Manager(conversation).SetToolsAttached(uuid, /*tools_attached=*/true);
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "fetch"));
}

TEST_F(MCPServerAssociatedContentTest, GetToolInfosListsPermissions) {
  mojom::MCPServerPtr server = AddServer(
      "Server", "https://mcp.example.com/mcp", {"search", "fetch", "delete"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");

  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 3u);
  for (const mojom::ToolInfoPtr& info : infos) {
    EXPECT_EQ(info->permission, mojom::ToolPermission::kAsk);
  }

  // Recorded by the name the user is shown, not the one the model calls.
  Manager(conversation)
      .SetToolPermission(uuid, "search", /*tool_description=*/"",
                         mojom::ToolPermission::kAllowSession);
  Manager(conversation)
      .SetToolPermission(uuid, "delete", /*tool_description=*/"",
                         mojom::ToolPermission::kNeverAllow);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 3u);
  EXPECT_EQ(infos[0]->name, "search");
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);
  EXPECT_EQ(infos[1]->name, "fetch");
  EXPECT_EQ(infos[1]->permission, mojom::ToolPermission::kAsk);
  EXPECT_EQ(infos[2]->name, "delete");
  EXPECT_EQ(infos[2]->permission, mojom::ToolPermission::kNeverAllow);

  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "fetch"));
  mojom::ToolUseEvent tool_use;
  EXPECT_EQ(
      std::get<bool>(GetLoopTool(conversation, "search")
                         ->RequiresUserInteractionBeforeHandling(tool_use)),
      false);
  EXPECT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(
      GetLoopTool(conversation, "fetch")
          ->RequiresUserInteractionBeforeHandling(tool_use)));
}

TEST_F(MCPServerAssociatedContentTest, PermissionsResetOnDetach) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  tools_[GURL(kEndpoint).spec()] =
      ToolsListResult({"search"}, "Searches the web.");
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  Manager(conversation)
      .SetToolPermission(uuid, "search", infos[0]->description,
                         mojom::ToolPermission::kAllowSession);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  ASSERT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);

  Detach(conversation, "Server");
  Attach(conversation, *server);
  infos = GetToolInfos(conversation, GetMCPServerUuid(conversation, "Server"));
  ASSERT_EQ(infos.size(), 1u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
}

// A gateway can host servers by different authors at one origin, so a choice
// about one server's tool mustn't apply to another's of the same name.
TEST_F(MCPServerAssociatedContentTest, ServersSharingAnOriginKeepPermissions) {
  mojom::MCPServerPtr first =
      AddServer("First", "https://gateway.example.com/first/mcp", {"search"});
  mojom::MCPServerPtr second =
      AddServer("Second", "https://gateway.example.com/second/mcp", {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *first);
  Attach(conversation, *second);
  const std::string first_uuid = GetMCPServerUuid(conversation, "First");
  const std::string second_uuid = GetMCPServerUuid(conversation, "Second");

  Manager(conversation)
      .SetToolPermission(first_uuid, "search", /*tool_description=*/"",
                         mojom::ToolPermission::kAllowSession);
  EXPECT_EQ(GetToolInfos(conversation, first_uuid)[0]->permission,
            mojom::ToolPermission::kAllowSession);
  EXPECT_EQ(GetToolInfos(conversation, second_uuid)[0]->permission,
            mojom::ToolPermission::kAsk);

  // Detaching one leaves the other's choices alone.
  Detach(conversation, "Second");
  EXPECT_EQ(GetToolInfos(conversation, first_uuid)[0]->permission,
            mojom::ToolPermission::kAllowSession);
}

// Allowing one server's tool for the session doesn't allow another's of the
// same name at the same origin.
TEST_F(MCPServerAssociatedContentTest, AllowSessionIsForOneServerOfAnOrigin) {
  mojom::MCPServerPtr first =
      AddServer("First", "https://gateway.example.com/first/mcp", {"search"});
  mojom::MCPServerPtr second =
      AddServer("Second", "https://gateway.example.com/second/mcp", {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  conversation->SetEngineForTesting(
      std::make_unique<NiceMock<MockEngineConsumer>>());
  Attach(conversation, *first);
  Attach(conversation, *second);
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "search"));
  base::WeakPtr<Tool> first_tool =
      GetLoopToolOfServer(conversation, *first, "search");
  ASSERT_TRUE(first_tool);
  SetPermissionChallenge(conversation, first_tool->Name());

  conversation->ProcessPermissionChallenge(
      "tool-use", mojom::PermissionChallengeDecision::kAllowSession);
  EXPECT_EQ(
      GetToolInfos(conversation, GetMCPServerUuid(conversation, "First"))[0]
          ->permission,
      mojom::ToolPermission::kAllowSession);
  EXPECT_EQ(
      GetToolInfos(conversation, GetMCPServerUuid(conversation, "Second"))[0]
          ->permission,
      mojom::ToolPermission::kAsk);
}

// The user allowed the tool as the server described it, so once the server
// describes it differently, the user is asked again.
TEST_F(MCPServerAssociatedContentTest, ChangedDescriptionAsksAgain) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  tools_[GURL(kEndpoint).spec()] =
      ToolsListResult({"search"}, "Searches the web.");
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  conversation->SetEngineForTesting(
      std::make_unique<NiceMock<MockEngineConsumer>>());
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
  SetPermissionChallenge(conversation,
                         GetLoopTool(conversation, "search")->Name());

  conversation->ProcessPermissionChallenge(
      "tool-use", mojom::PermissionChallengeDecision::kAllowSession);
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  EXPECT_EQ(infos[0]->description, "Searches the web.");
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);

  // Listed again once the client's cache expires.
  tools_[GURL(kEndpoint).spec()] =
      ToolsListResult({"search"}, "Deletes your files.");
  task_environment_.FastForwardBy(kMCPToolsCacheTtl);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  EXPECT_EQ(infos[0]->description, "Deletes your files.");
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
  mojom::ToolUseEvent tool_use;
  EXPECT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(
      GetLoopTool(conversation, "search")
          ->RequiresUserInteractionBeforeHandling(tool_use)));
}

// A tampered id can't make a server's permissions those of a real origin.
TEST_F(MCPServerAssociatedContentTest, NoPermissionsForAnIdThatIsNotAUuid) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  AddServer("Server", kEndpoint, {"search"});
  tools_[GURL(kEndpoint).spec()] =
      ToolsListResult({"search"}, "Searches the web.");
  {
    ScopedDictPrefUpdate update(&prefs_, prefs::kBraveAIChatMCPServers);
    update->FindList("servers")->front().GetDict().Set("id", "a.example.com/");
  }
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  ai_chat_service_->AssociateMCPServer("a.example.com/",
                                       conversation->get_conversation_uuid());
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  // With the description the user is shown, so that only the origin can keep
  // the choice from being recorded.
  Manager(conversation)
      .SetToolPermission(uuid, "search", infos[0]->description,
                         mojom::ToolPermission::kAllowSession);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
}

TEST_F(MCPServerAssociatedContentTest, CapsToolsPerServer) {
  mojom::MCPServerPtr server = AddServer(
      "Server", "https://mcp.example.com/mcp", NumberedToolNames("tool", 40));
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);

  // The website tools dialog lists only what the model is offered.
  EXPECT_EQ(GetToolInfos(conversation, GetMCPServerUuid(conversation, "Server"))
                .size(),
            30u);
  std::vector<std::string> tools = RunGenerationLoop(conversation);
  EXPECT_EQ(tools.size(), 30u);
  EXPECT_EQ(tools.back(), "tool_29");
}

TEST_F(MCPServerAssociatedContentTest, CapsToolsAcrossServers) {
  mojom::MCPServerPtr first = AddServer(
      "First", "https://first.example.com/mcp", NumberedToolNames("first", 30));
  mojom::MCPServerPtr second =
      AddServer("Second", "https://second.example.com/mcp",
                NumberedToolNames("second", 30));
  mojom::MCPServerPtr third = AddServer(
      "Third", "https://third.example.com/mcp", NumberedToolNames("third", 30));
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *first);
  Attach(conversation, *second);
  Attach(conversation, *third);
  // Other content's tools don't count against the servers' cap.
  NiceMock<MockAssociatedContent> page;
  page.SetUrl(GURL("https://page.example.com"));
  EXPECT_CALL(page, GetContentTools)
      .WillRepeatedly(
          [](AssociatedContentDelegate::GetContentToolsCallback cb) {
            std::vector<std::unique_ptr<Tool>> tools;
            tools.push_back(std::make_unique<NiceMock<MockTool>>("page_tool"));
            std::move(cb).Run(std::move(tools));
          });
  Manager(conversation).AddContent(&page);
  ASSERT_TRUE(page.tools_attached());

  // The servers attached first keep their tools.
  std::vector<std::string> expected = NumberedToolNames("first", 30);
  for (std::string& name : NumberedToolNames("second", 30)) {
    expected.push_back(std::move(name));
  }
  expected.push_back("page_tool");
  EXPECT_EQ(RunGenerationLoop(conversation), expected);
  EXPECT_EQ(expected.size(), kMaxMCPToolsPerGenerationLoop + 1);

  // The tools listed for each server are those the model is offered.
  EXPECT_EQ(GetToolInfos(conversation, GetMCPServerUuid(conversation, "Second"))
                .size(),
            30u);
  EXPECT_TRUE(
      GetToolInfos(conversation, GetMCPServerUuid(conversation, "Third"))
          .empty());

  Manager(conversation).RemoveContent(&page);
}

// Detaching a server moves where the cap cuts the tools of those after it, and
// their tool lists are pushed again.
TEST_F(MCPServerAssociatedContentTest, CapMovesWhenAnEarlierServerIsDetached) {
  mojom::MCPServerPtr first = AddServer(
      "First", "https://first.example.com/mcp", NumberedToolNames("first", 30));
  mojom::MCPServerPtr second =
      AddServer("Second", "https://second.example.com/mcp",
                NumberedToolNames("second", 30));
  mojom::MCPServerPtr third = AddServer(
      "Third", "https://third.example.com/mcp", NumberedToolNames("third", 30));
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Attach(conversation, *first);
  Attach(conversation, *second);
  Attach(conversation, *third);
  const std::string third_uuid = GetMCPServerUuid(conversation, "Third");
  ASSERT_TRUE(GetToolInfos(conversation, third_uuid).empty());

  Detach(conversation, "First");
  ui.WaitForPushedTools(third_uuid, NumberedToolNames("third", 30));
}

TEST_F(MCPServerAssociatedContentTest, ServerRemovedWhileAttached) {
  mojom::MCPServerPtr server =
      AddServer("Server", "https://mcp.example.com/mcp", {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
  base::WeakPtr<Tool> tool = GetLoopTool(conversation, "search");

  base::test::TestFuture<std::optional<mojom::MCPServerError>> removed;
  host().RemoveServer(server->id, removed.GetCallback());
  ASSERT_EQ(removed.Take(), std::nullopt);

  // It stays attached, but offers nothing.
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Server"));
  EXPECT_TRUE(
      GetToolInfos(conversation, GetMCPServerUuid(conversation, "Server"))
          .empty());
  // A tool from before fails rather than hangs.
  ASSERT_TRUE(tool);
  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result;
  tool->UseTool("{}", result.GetCallback());
  ASSERT_EQ(std::get<0>(result.Get()).size(), 1u);
  EXPECT_EQ(std::get<0>(result.Get())[0]->get_text_content_block()->text,
            "Server is no longer available.");
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());
}

TEST_F(MCPServerAssociatedContentTest, ServerChangedWhileAttached) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));

  // Turned off, it offers nothing until it's turned back on.
  UpdateServer(*server, "Server", kEndpoint, /*enabled=*/false);
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());
  UpdateServer(*server, "Server", kEndpoint, /*enabled=*/true);
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));

  // Renaming it renames the attachment.
  UpdateServer(*server, "Renamed", kEndpoint, /*enabled=*/true);
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Renamed"));
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));

  // It was attached at its origin, so it offers nothing once it's moved to
  // another.
  tools_[GURL("https://elsewhere.example.com/mcp").spec()] =
      ToolsListResult({"search"});
  UpdateServer(*server, "Renamed", "https://elsewhere.example.com/mcp",
               /*enabled=*/true);
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());
}

// Choices about a server's tools were made at its endpoint, so they don't
// follow it to another path at the same origin, where another author's server
// can be. The server keeps offering its tools at that path.
TEST_F(MCPServerAssociatedContentTest,
       PermissionsDontFollowTheServerToAnotherPath) {
  constexpr char kEndpoint[] = "https://gateway.example.com/first/mcp";
  constexpr char kMovedEndpoint[] = "https://gateway.example.com/second/mcp";
  mojom::MCPServerPtr server =
      AddServer("Server", kEndpoint, {"search", "delete"});
  tools_[GURL(kMovedEndpoint).spec()] = ToolsListResult({"search", "delete"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  Manager(conversation)
      .SetToolPermission(uuid, "search", /*tool_description=*/"",
                         mojom::ToolPermission::kAllowSession);
  Manager(conversation)
      .SetToolPermission(uuid, "delete", /*tool_description=*/"",
                         mojom::ToolPermission::kNeverAllow);
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 2u);
  ASSERT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);
  ASSERT_EQ(infos[1]->permission, mojom::ToolPermission::kNeverAllow);
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));

  ui.ClearPushedTools();
  UpdateServer(*server, "Server", kMovedEndpoint, /*enabled=*/true);
  // The UI keeps showing its tools until they're listed at the new path.
  ui.WaitForPushedTools(uuid, {"search", "delete"});
  EXPECT_FALSE(ui.WasPushed(uuid, {}));
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 2u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
  EXPECT_EQ(infos[1]->permission, mojom::ToolPermission::kAsk);
  // The tool that was blocked is offered, and the one that was allowed is
  // asked about.
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "delete"));
  mojom::ToolUseEvent tool_use;
  EXPECT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(
      GetLoopTool(conversation, "search")
          ->RequiresUserInteractionBeforeHandling(tool_use)));
}

// Choices reset by a move to another path at the same origin don't come back
// when the server is moved back to the path they were made at.
TEST_F(MCPServerAssociatedContentTest, PermissionsDontComeBackWithTheServer) {
  constexpr char kEndpoint[] = "https://gateway.example.com/first/mcp";
  constexpr char kMovedEndpoint[] = "https://gateway.example.com/second/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  tools_[GURL(kMovedEndpoint).spec()] = ToolsListResult({"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  Manager(conversation)
      .SetToolPermission(uuid, "search", /*tool_description=*/"",
                         mojom::ToolPermission::kAllowSession);
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  ASSERT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);

  UpdateServer(*server, "Server", kMovedEndpoint, /*enabled=*/true);
  UpdateServer(*server, "Server", kEndpoint, /*enabled=*/true);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
}

// A prompt answered once the server has moved to another path was about the
// tool at the path before, so allowing it for the chat doesn't allow the tool
// of the same name and description at the new path.
TEST_F(MCPServerAssociatedContentTest,
       AnswerFromBeforeAMoveIsNotRecordedForTheNewPath) {
  constexpr char kEndpoint[] = "https://gateway.example.com/first/mcp";
  constexpr char kMovedEndpoint[] = "https://gateway.example.com/second/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  tools_[GURL(kEndpoint).spec()] =
      ToolsListResult({"search"}, "Searches the web.");
  tools_[GURL(kMovedEndpoint).spec()] =
      ToolsListResult({"search"}, "Searches the web.");
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  conversation->SetEngineForTesting(
      std::make_unique<NiceMock<MockEngineConsumer>>());
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
  SetPermissionChallenge(conversation,
                         GetLoopTool(conversation, "search")->Name());

  UpdateServer(*server, "Server", kMovedEndpoint, /*enabled=*/true);
  conversation->ProcessPermissionChallenge(
      "tool-use", mojom::PermissionChallengeDecision::kAllowSession);
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 1u);
  EXPECT_EQ(infos[0]->description, "Searches the web.");
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
}

// It was attached at its origin, so it offers nothing at another. Moved back,
// it's used again, with the choices made about its tools reset.
TEST_F(MCPServerAssociatedContentTest, UsedAgainOnceMovedBackToItsOrigin) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  constexpr char kElsewhere[] = "https://elsewhere.example.com/mcp";
  mojom::MCPServerPtr server =
      AddServer("Server", kEndpoint, {"search", "delete"});
  tools_[GURL(kElsewhere).spec()] = ToolsListResult({"search", "delete"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  Manager(conversation)
      .SetToolPermission(uuid, "search", /*tool_description=*/"",
                         mojom::ToolPermission::kAllowSession);
  Manager(conversation)
      .SetToolPermission(uuid, "delete", /*tool_description=*/"",
                         mojom::ToolPermission::kNeverAllow);
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 2u);
  ASSERT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);
  ASSERT_EQ(infos[1]->permission, mojom::ToolPermission::kNeverAllow);
  ASSERT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));

  ui.ClearPushedTools();
  UpdateServer(*server, "Server", kElsewhere, /*enabled=*/true);
  EXPECT_TRUE(ui.WasPushed(uuid, {}));
  EXPECT_TRUE(GetToolInfos(conversation, uuid).empty());
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());

  UpdateServer(*server, "Server", kEndpoint, /*enabled=*/true);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 2u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAsk);
  EXPECT_EQ(infos[1]->permission, mojom::ToolPermission::kAsk);
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "delete"));
}

// A new name or token doesn't make it another server, so the choices made about
// its tools stand.
TEST_F(MCPServerAssociatedContentTest,
       PermissionsStayWhenRenamedOrGivenAToken) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server =
      AddServer("Server", kEndpoint, {"search", "delete"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  Manager(conversation)
      .SetToolPermission(uuid, "search", /*tool_description=*/"",
                         mojom::ToolPermission::kAllowSession);
  Manager(conversation)
      .SetToolPermission(uuid, "delete", /*tool_description=*/"",
                         mojom::ToolPermission::kNeverAllow);

  // Each replaces the server's client.
  base::WeakPtr<MCPClient> client = host().GetClient(server->id);
  UpdateServer(*server, "Renamed", kEndpoint, /*enabled=*/true);
  ASSERT_FALSE(client);
  std::vector<mojom::ToolInfoPtr> infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 2u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);
  EXPECT_EQ(infos[1]->permission, mojom::ToolPermission::kNeverAllow);

  client = host().GetClient(server->id);
  UpdateServer(*server, "Renamed", kEndpoint, /*enabled=*/true, "token");
  ASSERT_FALSE(client);
  infos = GetToolInfos(conversation, uuid);
  ASSERT_EQ(infos.size(), 2u);
  EXPECT_EQ(infos[0]->permission, mojom::ToolPermission::kAllowSession);
  EXPECT_EQ(infos[1]->permission, mojom::ToolPermission::kNeverAllow);
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
}

// Only the endpoint's origin is shown, as a server's path or query can hold a
// key.
TEST_F(MCPServerAssociatedContentTest, OnlyTheEndpointOriginIsShown) {
  mojom::MCPServerPtr server = AddServer(
      "Server", "https://mcp.example.com/key/secret/mcp?api_key=secret#ref",
      {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  EXPECT_EQ(GetMCPServers(conversation)[0]->url,
            GURL("https://mcp.example.com/"));
}

TEST_F(MCPServerAssociatedContentTest, ContentUsedPercentageIsThePages) {
  AddServer("Auto", "https://auto.example.com/mcp", {"search"},
            /*enabled=*/true, /*auto_attach=*/true);
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  const uint32_t max_length =
      ModelService::CalcuateMaxAssociatedContentLengthForModel(
          conversation->GetCurrentModel());
  NiceMock<MockAssociatedContent> page;
  page.SetUrl(GURL("https://page.example.com"));
  page.SetTextContent(std::string(max_length * 2, 'a'));
  Manager(conversation).AddContent(&page);

  std::vector<mojom::AssociatedContentPtr> content =
      Manager(conversation).GetAssociatedContent();
  ASSERT_EQ(content.size(), 2u);
  EXPECT_EQ(content[0]->content_type, mojom::ContentType::MCPServer);
  EXPECT_EQ(content[0]->content_used_percentage, 100);
  // The server takes nothing from the page's share.
  const int expected =
      base::ClampRound(static_cast<float>(max_length) /
                       static_cast<float>(max_length * 2 + 15) * 100);
  EXPECT_LT(expected, 100);
  EXPECT_EQ(content[1]->content_used_percentage, expected);
  EXPECT_EQ(conversation->GetStateForConversationEntriesForTesting()
                ->content_used_percentage,
            static_cast<uint32_t>(expected));
  Manager(conversation).RemoveContent(&page);
}

// Attachments are live only: nothing about them is stored, so a reloaded
// conversation has only the servers attached to every conversation.
TEST_F(MCPServerAssociatedContentTest, IsNeverStored) {
  AddServer("Auto", "https://auto.example.com/mcp", {"search"},
            /*enabled=*/true, /*auto_attach=*/true);
  mojom::MCPServerPtr manual =
      AddServer("Manual", "https://manual.example.com/mcp", {"search"});
  auto page = std::make_unique<NiceMock<MockAssociatedContent>>();
  page->SetUrl(GURL("https://page.example.com"));
  page->SetTitle(u"Page");
  page->SetTextContent("Page text");

  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  auto ui = std::make_unique<TestConversationUI>(*conversation);
  const std::string conversation_uuid = conversation->get_conversation_uuid();
  Attach(conversation, *manual);
  Manager(conversation).AddContent(page.get());
  ASSERT_THAT(GetMCPServerTitles(conversation), ElementsAre("Auto", "Manual"));

  // Content goes with the turn it was sent with, except for servers.
  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(HumanTurn("turn"));
  conversation->SetChatHistoryForTesting(std::move(history));
  for (const mojom::AssociatedContentPtr& content :
       Manager(conversation).GetAssociatedContent()) {
    EXPECT_EQ(content->conversation_turn_uuid,
              content->content_type == mojom::ContentType::MCPServer
                  ? std::nullopt
                  : std::optional<std::string>("turn"));
  }
  PageContentsMap contents = Manager(conversation).GetCachedContentsMap();
  ASSERT_EQ(contents.size(), 1u);
  ASSERT_EQ(contents["turn"].size(), 1u);
  EXPECT_EQ(contents["turn"][0].get().content, "Page text");

  // Only the page is stored.
  base::test::TestFuture<std::vector<mojom::ConversationPtr>> stored;
  ai_chat_service_->GetConversations(stored.GetCallback());
  ASSERT_EQ(stored.Get().size(), 1u);
  ASSERT_EQ(stored.Get()[0]->associated_content.size(), 1u);
  EXPECT_EQ(stored.Get()[0]->associated_content[0]->content_type,
            mojom::ContentType::PageContent);

  // Unloaded once its tab is closed and nothing shows it, it's loaded again
  // from storage.
  page.reset();
  ui.reset();
  ASSERT_TRUE(base::test::RunUntil([&] {
    return ai_chat_service_->GetInMemoryConversationCountForTesting() == 0;
  }));
  base::test::TestFuture<ConversationHandler*> loaded;
  ai_chat_service_->GetConversation(conversation_uuid, loaded.GetCallback());
  conversation = loaded.Get();
  ASSERT_TRUE(conversation);
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Auto"));
  EXPECT_TRUE(Manager(conversation).HasPageContent());
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
}

// What another version of the browser might have stored isn't restored.
TEST_F(MCPServerAssociatedContentTest, StoredServersAreNotRestored) {
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  mojom::ConversationPtr metadata = mojom::Conversation::New();
  metadata->uuid = conversation->get_conversation_uuid();
  for (auto [uuid, type] :
       {std::pair("page", mojom::ContentType::PageContent),
        std::pair("server", mojom::ContentType::MCPServer)}) {
    auto content = mojom::AssociatedContent::New();
    content->uuid = uuid;
    content->content_type = type;
    content->title = uuid;
    content->url = GURL("https://example.com/mcp");
    content->conversation_turn_uuid = "turn";
    metadata->associated_content.push_back(std::move(content));
  }
  mojom::ConversationArchivePtr archive = mojom::ConversationArchive::New();
  archive->associated_content.push_back(
      mojom::ContentArchive::New("page", "Page text", "turn"));
  archive->associated_content.push_back(
      mojom::ContentArchive::New("server", "", "turn"));

  Manager(conversation).LoadArchivedContent(metadata.get(), archive);
  std::vector<mojom::AssociatedContentPtr> content =
      Manager(conversation).GetAssociatedContent();
  ASSERT_EQ(content.size(), 1u);
  EXPECT_EQ(content[0]->uuid, "page");
}

// Reloading archived content keeps the servers that are attached.
TEST_F(MCPServerAssociatedContentTest, ArchiveReloadKeepsAttachedServers) {
  mojom::MCPServerPtr server =
      AddServer("Server", "https://mcp.example.com/mcp", {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  Attach(conversation, *server);
  mojom::ConversationPtr metadata = mojom::Conversation::New();
  metadata->uuid = conversation->get_conversation_uuid();
  Manager(conversation)
      .LoadArchivedContent(metadata.get(), mojom::ConversationArchive::New());
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Server"));
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
}

// Every message waits for the servers' tools, but only so long.
TEST_F(MCPServerAssociatedContentTest, SlowServerDoesNotHoldUpTheLoop) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Hold(kEndpoint);
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");

  base::test::TestFuture<void> done;
  Manager(conversation).UpdateToolsForNewGenerationLoop(done.GetCallback());
  task_environment_.FastForwardBy(kMCPToolsDeadline - base::Milliseconds(1));
  EXPECT_FALSE(done.IsReady());
  task_environment_.FastForwardBy(base::Milliseconds(1));
  ASSERT_TRUE(done.IsReady());
  EXPECT_TRUE(GetLoopToolNames(conversation).empty());

  // Tools listed later are shown, and added to the rest of the loop.
  Release(kEndpoint);
  ui.WaitForPushedTools(uuid, {"search"});
  EXPECT_THAT(GetLoopToolNames(conversation), ElementsAre("search"));

  // Once its tools have been listed, a loop that stops waiting for the server
  // has the tools it listed last.
  tools_[GURL(kEndpoint).spec()] = ToolsListResult({"search", "fetch"});
  task_environment_.FastForwardBy(kMCPToolsCacheTtl);
  Hold(kEndpoint);
  base::test::TestFuture<void> second_done;
  Manager(conversation)
      .UpdateToolsForNewGenerationLoop(second_done.GetCallback());
  task_environment_.FastForwardBy(kMCPToolsDeadline);
  ASSERT_TRUE(second_done.IsReady());
  EXPECT_THAT(GetLoopToolNames(conversation), ElementsAre("search"));

  // The later answer is shown, but the loop already has the server's tools.
  Release(kEndpoint);
  ui.WaitForPushedTools(uuid, {"search", "fetch"});
  EXPECT_THAT(GetLoopToolNames(conversation), ElementsAre("search"));
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search", "fetch"));
}

TEST_F(MCPServerAssociatedContentTest, BacksOffAfterFailing) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  failing_endpoints_.insert(GURL(kEndpoint).spec());
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  EXPECT_TRUE(RunGenerationLoop(conversation).empty());

  // It isn't asked again for a while, so the loop doesn't wait for it.
  failing_endpoints_.clear();
  const size_t request_count = mcp_server_.RequestCount();
  base::test::TestFuture<void> done;
  Manager(conversation).UpdateToolsForNewGenerationLoop(done.GetCallback());
  EXPECT_TRUE(done.IsReady());
  EXPECT_EQ(mcp_server_.RequestCount(), request_count);

  // Once it lists its tools, they're shown.
  task_environment_.FastForwardBy(kMCPToolsRetryDelay);
  EXPECT_THAT(RunGenerationLoop(conversation), ElementsAre("search"));
  ui.WaitForPushedTools(uuid, {"search"});
}

// A hung server's requests time out, after which it's left alone.
TEST_F(MCPServerAssociatedContentTest, HungServerIsLeftAlone) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Hold(kEndpoint);
  Attach(conversation, *server);
  task_environment_.FastForwardBy(kMCPRequestTimeout);

  const size_t request_count = mcp_server_.RequestCount();
  base::test::TestFuture<void> done;
  Manager(conversation).UpdateToolsForNewGenerationLoop(done.GetCallback());
  EXPECT_TRUE(done.IsReady());
  EXPECT_TRUE(GetLoopToolNames(conversation).empty());
  EXPECT_EQ(mcp_server_.RequestCount(), request_count);
}

// A loop doesn't take tools from content detached while it waits, nor take
// them twice from content attached again meanwhile.
TEST_F(MCPServerAssociatedContentTest, DetachAndReattachWhileWaiting) {
  constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
  mojom::MCPServerPtr server = AddServer("Server", kEndpoint, {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  TestConversationUI ui(*conversation);
  Hold(kEndpoint);
  Attach(conversation, *server);
  NiceMock<MockAssociatedContent> page;
  page.SetUrl(GURL("https://page.example.com"));
  AssociatedContentDelegate::GetContentToolsCallback pending_page_tools;
  EXPECT_CALL(page, GetContentTools)
      .WillRepeatedly(
          [&](AssociatedContentDelegate::GetContentToolsCallback cb) {
            std::vector<std::unique_ptr<Tool>> tools;
            tools.push_back(std::make_unique<NiceMock<MockTool>>("page_tool"));
            if (!pending_page_tools && page.tools_attached()) {
              // The loop's fetch, held until the end.
              pending_page_tools = std::move(cb);
              return;
            }
            std::move(cb).Run(std::move(tools));
          });
  Manager(conversation).AddContent(&page);
  ASSERT_TRUE(page.tools_attached());
  pending_page_tools.Reset();

  base::test::TestFuture<void> done;
  Manager(conversation).UpdateToolsForNewGenerationLoop(done.GetCallback());
  ASSERT_TRUE(pending_page_tools);
  Detach(conversation, "Server");
  Attach(conversation, *server);
  const std::string uuid = GetMCPServerUuid(conversation, "Server");
  Manager(conversation).SetToolsAttached(page.uuid(), false);
  Manager(conversation).SetToolsAttached(page.uuid(), true);
  Release(kEndpoint);
  ui.WaitForPushedTools(uuid, {"search"});
  EXPECT_THAT(GetLoopToolNames(conversation),
              UnorderedElementsAre("search", "page_tool"));
  EXPECT_FALSE(done.IsReady());

  std::vector<std::unique_ptr<Tool>> tools;
  tools.push_back(std::make_unique<NiceMock<MockTool>>("page_tool"));
  std::move(pending_page_tools).Run(std::move(tools));
  EXPECT_TRUE(done.Wait());
  EXPECT_THAT(GetLoopToolNames(conversation),
              UnorderedElementsAre("search", "page_tool"));
  Manager(conversation).RemoveContent(&page);
}

// A conversation created before tokens can be decrypted gets its servers once
// they can.
TEST_F(MCPServerAssociatedContentTest, AutoAttachesOnceServersAreReadable) {
  AddServer("Auto", "https://auto.example.com/mcp", {"search"},
            /*enabled=*/true, /*auto_attach=*/true);
  DeferredOSCryptAsync deferred_os_crypt(
      os_crypt_async::GetTestEncryptorForTesting());
  ResetService(deferred_os_crypt);

  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  EXPECT_TRUE(GetMCPServers(conversation).empty());
  deferred_os_crypt.Complete();
  WaitUntilReady(host());
  EXPECT_THAT(GetMCPServerTitles(conversation), ElementsAre("Auto"));

  // Replaced while `deferred_os_crypt` is alive.
  ResetService();
}

TEST_F(MCPServerAssociatedContentTest, NoFetchForAModelWithoutTools) {
  mojom::MCPServerPtr server =
      AddServer("Server", "https://mcp.example.com/mcp", {"search"});
  ConversationHandler* conversation = ai_chat_service_->CreateConversation();
  conversation->ChangeModel("chat-qwen");
  ASSERT_FALSE(conversation->GetCurrentModel().supports_tools);
  Attach(conversation, *server);

  base::test::TestFuture<void> done;
  Manager(conversation).UpdateToolsForNewGenerationLoop(done.GetCallback());
  EXPECT_TRUE(done.IsReady());
  EXPECT_TRUE(GetLoopToolNames(conversation).empty());
  EXPECT_EQ(mcp_server_.RequestCount(), 0u);
}

}  // namespace ai_chat
