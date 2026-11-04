/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_host.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_http_test_base.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_server_registry.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_test_server.h"
#include "brave/components/ai_chat/core/browser/mcp/test/deferred_os_crypt_async.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
constexpr char kToken[] = "secret-token-1234";

mojom::MCPServerParamsPtr Params(std::string_view name = "Example",
                                 std::string_view endpoint = kEndpoint,
                                 bool enabled = true,
                                 bool auto_attach = false) {
  return mojom::MCPServerParams::New(std::string(name), GURL(endpoint), enabled,
                                     auto_attach);
}

using ServerResult = base::expected<mojom::MCPServerPtr, mojom::MCPServerError>;
using ToolsResult =
    base::expected<std::vector<mojom::MCPServerToolPtr>, mojom::MCPServerError>;

class TestObserver : public MCPHost::Observer {
 public:
  explicit TestObserver(MCPHost& host) : host_(host) {}

  void OnMCPClientsChanged() override {
    // Clients have already been updated.
    enabled_server_counts.push_back(host_->GetEnabledServers().size());
  }
  void OnMCPServerStatusChanged(const std::string& server_id) override {
    statuses.push_back(host_->GetServerStatus(server_id));
  }

  std::vector<size_t> enabled_server_counts;
  // Each status a server was changed to.
  std::vector<mojom::MCPServerStatus> statuses;

 private:
  raw_ref<MCPHost> host_;
};

using Statuses = base::flat_map<std::string, mojom::MCPServerStatus>;

class TestMojomObserver : public mojom::MCPHostObserver {
 public:
  void OnServersChanged(std::vector<mojom::MCPServerPtr> servers) override {
    future.SetValue(std::move(servers));
  }
  void OnServerStatusesChanged(const Statuses& statuses) override {
    pushed_statuses.push_back(statuses);
  }

  base::test::TestFuture<std::vector<mojom::MCPServerPtr>> future;
  std::vector<Statuses> pushed_statuses;
  mojo::Receiver<mojom::MCPHostObserver> receiver{this};
};

}  // namespace

class MCPHostTest : public testing::Test {
 protected:
  MCPHostTest() {
    prefs::RegisterProfilePrefs(prefs_.registry());
    host_ = std::make_unique<MCPHost>(prefs_, os_crypt_,
                                      url_loader_factory_.GetSafeWeakWrapper());
    host_->Bind(remote_.BindNewPipeAndPassReceiver());
  }

  void TearDown() override {
    host_.reset();
    task_environment_.FastForwardUntilNoTasksRemain();
  }

  void RunReadyTasks() { task_environment_.FastForwardBy(base::TimeDelta()); }

  mojom::MCPServerPtr Add(mojom::MCPServerParamsPtr params = Params(),
                          std::optional<std::string> token = std::nullopt) {
    base::test::TestFuture<ServerResult> future;
    remote_->AddServer(std::move(params), std::move(token),
                       future.GetCallback());
    ServerResult result = future.Take();
    EXPECT_TRUE(result.has_value());
    return result.has_value() ? std::move(*result) : nullptr;
  }

  ServerResult Update(const std::string& id,
                      mojom::MCPServerParamsPtr params,
                      std::optional<std::string> token = std::nullopt) {
    base::test::TestFuture<ServerResult> future;
    remote_->UpdateServer(id, std::move(params), std::move(token),
                          future.GetCallback());
    return future.Take();
  }

  std::optional<mojom::MCPServerError> Remove(const std::string& id) {
    base::test::TestFuture<std::optional<mojom::MCPServerError>> future;
    remote_->RemoveServer(id, future.GetCallback());
    return future.Get();
  }

  std::vector<mojom::MCPServerPtr> GetServers() {
    base::test::TestFuture<std::vector<mojom::MCPServerPtr>> future;
    remote_->GetServers(future.GetCallback());
    return future.Take();
  }

  Statuses GetServerStatuses() {
    base::test::TestFuture<Statuses> future;
    remote_->GetServerStatuses(future.GetCallback<const Statuses&>());
    return future.Take();
  }

  void Reconnect(const std::string& id) {
    remote_->ReconnectServer(id);
    remote_.FlushForTesting();
  }

  // Lists the tools of server `id`, and fails the `initialize` at `index`,
  // which the listing sends.
  void FailListing(const std::string& id, size_t index) {
    host_->GetTools(id, base::DoNothing());
    FailInitialize(index);
  }

  void FailInitialize(size_t index) {
    server_requests_.WaitForRequest(index);
    ASSERT_EQ(server_requests_.Method(index), "initialize");
    server_requests_.RespondError(index, -32603, "Internal error");
    RunReadyTasks();
  }

  // Answers the `initialize` at `index`, and the `tools/list` after it.
  void ListTools(size_t index) {
    const size_t list = server_requests_.CompleteHandshake(index);
    server_requests_.WaitForRequest(list);
    ASSERT_EQ(server_requests_.Method(list), "tools/list");
    server_requests_.RespondResult(list, R"({"tools":[]})");
    RunReadyTasks();
  }

  void RespondWithStatus(size_t index, int status) {
    url_loader_factory_.SimulateResponseWithoutRemovingFromPendingList(
        &url_loader_factory_.pending_requests()->at(index),
        MakeMCPResponseHead(status, "text/plain"), "",
        network::URLLoaderCompletionStatus(net::OK));
    RunReadyTasks();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple prefs_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_ =
      os_crypt_async::GetTestEncryptorForTesting();
  DeferredOSCryptAsync os_crypt_{encryptor_};
  network::TestURLLoaderFactory url_loader_factory_;
  MCPTestServer server_requests_{url_loader_factory_};
  std::unique_ptr<MCPHost> host_;
  mojo::Remote<mojom::MCPHost> remote_;
};

TEST_F(MCPHostTest, WaitsForTokensToBeDecryptable) {
  // Added before this profile's host could decrypt tokens.
  DeferredOSCryptAsync other_os_crypt(encryptor_);
  other_os_crypt.Complete();
  MCPServerRegistry other_registry(prefs_, other_os_crypt);
  mojom::MCPServerPtr server =
      other_registry.AddServer(*Params(), std::string(kToken)).value();

  base::test::TestFuture<std::vector<mojom::MCPServerPtr>> servers;
  remote_->GetServers(servers.GetCallback());
  RunReadyTasks();
  EXPECT_FALSE(servers.IsReady());
  EXPECT_FALSE(host_->GetClient(server->id));

  os_crypt_.Complete();
  std::vector<mojom::MCPServerPtr> listed = servers.Take();
  ASSERT_EQ(listed.size(), 1u);
  EXPECT_EQ(*listed[0], *server);
  EXPECT_TRUE(host_->GetClient(server->id));
}

TEST_F(MCPHostTest, ClientsFollowEnabledState) {
  os_crypt_.Complete();
  mojom::MCPServerPtr enabled = Add(Params("Enabled"));
  mojom::MCPServerPtr disabled =
      Add(Params("Disabled", kEndpoint, /*enabled=*/false));
  base::WeakPtr<MCPClient> client = host_->GetClient(enabled->id);
  EXPECT_TRUE(client);
  EXPECT_FALSE(host_->GetClient(disabled->id));

  ASSERT_TRUE(
      Update(enabled->id, Params("Enabled", kEndpoint, /*enabled=*/false))
          .has_value());
  EXPECT_FALSE(client);
  EXPECT_FALSE(host_->GetClient(enabled->id));

  ASSERT_TRUE(Update(disabled->id, Params("Disabled")).has_value());
  client = host_->GetClient(disabled->id);
  EXPECT_TRUE(client);

  EXPECT_EQ(Remove(disabled->id), std::nullopt);
  EXPECT_FALSE(client);
  EXPECT_FALSE(host_->GetClient(disabled->id));
}

TEST_F(MCPHostTest, ConfigChangesReplaceClient) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add(Params("Name"));
  base::WeakPtr<MCPClient> client = host_->GetClient(server->id);
  ASSERT_TRUE(client);

  // Not something a client uses.
  ASSERT_TRUE(Update(server->id, Params("Name", kEndpoint, /*enabled=*/true,
                                        /*auto_attach=*/true))
                  .has_value());
  EXPECT_TRUE(client);

  ASSERT_TRUE(Update(server->id, Params("Name", "https://other.example.com/"))
                  .has_value());
  EXPECT_FALSE(client);
  client = host_->GetClient(server->id);
  ASSERT_TRUE(client);

  ASSERT_TRUE(
      Update(server->id, Params("Name", "https://other.example.com/"), kToken)
          .has_value());
  EXPECT_FALSE(client);
  client = host_->GetClient(server->id);
  ASSERT_TRUE(client);

  // The name is in the client's error messages.
  ASSERT_TRUE(
      Update(server->id, Params("Renamed", "https://other.example.com/"))
          .has_value());
  EXPECT_FALSE(client);
  client = host_->GetClient(server->id);
  ASSERT_TRUE(client);

  base::test::TestFuture<ServerResult> cleared;
  remote_->ClearServerToken(server->id, cleared.GetCallback());
  ASSERT_TRUE(cleared.Take().has_value());
  EXPECT_FALSE(client);
  EXPECT_TRUE(host_->GetClient(server->id));
}

TEST_F(MCPHostTest, Crud) {
  os_crypt_.Complete();
  EXPECT_TRUE(GetServers().empty());

  mojom::MCPServerPtr server = Add(Params("Example"), kToken);
  EXPECT_EQ(server->name, "Example");
  EXPECT_EQ(server->endpoint, GURL(kEndpoint));
  EXPECT_EQ(server->slug, "example");
  EXPECT_TRUE(server->has_token);
  std::vector<mojom::MCPServerPtr> servers = GetServers();
  ASSERT_EQ(servers.size(), 1u);
  EXPECT_EQ(*servers[0], *server);

  ServerResult updated =
      Update(server->id, Params("Renamed", kEndpoint, /*enabled=*/false));
  ASSERT_TRUE(updated.has_value());
  EXPECT_EQ((*updated)->name, "Renamed");
  EXPECT_FALSE((*updated)->enabled);
  // Kept, as no token was given.
  EXPECT_TRUE((*updated)->has_token);

  base::test::TestFuture<ServerResult> cleared;
  remote_->ClearServerToken(server->id, cleared.GetCallback());
  ServerResult cleared_result = cleared.Take();
  ASSERT_TRUE(cleared_result.has_value());
  EXPECT_FALSE((*cleared_result)->has_token);

  EXPECT_EQ(Remove(server->id), std::nullopt);
  EXPECT_EQ(Remove(server->id), mojom::MCPServerError::kNotFound);
  EXPECT_TRUE(GetServers().empty());
}

TEST_F(MCPHostTest, CrudErrors) {
  os_crypt_.Complete();
  base::test::TestFuture<ServerResult> added;
  remote_->AddServer(Params("Name", "http://mcp.example.com/"), std::nullopt,
                     added.GetCallback());
  EXPECT_EQ(added.Take(),
            base::unexpected(mojom::MCPServerError::kInsecureScheme));
  base::test::TestFuture<ServerResult> added_with_token;
  remote_->AddServer(Params(), "a\nb", added_with_token.GetCallback());
  EXPECT_EQ(added_with_token.Take(),
            base::unexpected(mojom::MCPServerError::kInvalidToken));

  EXPECT_EQ(Update("unknown", Params()),
            base::unexpected(mojom::MCPServerError::kNotFound));
  base::test::TestFuture<ServerResult> cleared;
  remote_->ClearServerToken("unknown", cleared.GetCallback());
  EXPECT_EQ(cleared.Take(), base::unexpected(mojom::MCPServerError::kNotFound));
  EXPECT_TRUE(GetServers().empty());
}

TEST_F(MCPHostTest, TokenIsNeverReturned) {
  os_crypt_.Complete();
  TestMojomObserver observer;
  remote_->BindObserver(observer.receiver.BindNewPipeAndPassRemote());

  auto expect_no_token = [](const mojom::MCPServer& server) {
    EXPECT_TRUE(server.has_token);
    for (const std::string& value :
         {server.id, server.name, server.slug, server.endpoint.spec()}) {
      EXPECT_EQ(value.find(kToken), std::string::npos);
    }
  };
  mojom::MCPServerPtr server = Add(Params(), kToken);
  expect_no_token(*server);
  std::vector<mojom::MCPServerPtr> changed = observer.future.Take();
  ASSERT_EQ(changed.size(), 1u);
  expect_no_token(*changed[0]);
  std::vector<mojom::MCPServerPtr> servers = GetServers();
  ASSERT_EQ(servers.size(), 1u);
  expect_no_token(*servers[0]);
  ServerResult updated = Update(server->id, Params("Renamed"));
  ASSERT_TRUE(updated.has_value());
  expect_no_token(**updated);
}

TEST_F(MCPHostTest, NotifiesObservers) {
  os_crypt_.Complete();
  TestMojomObserver mojom_observer;
  remote_->BindObserver(mojom_observer.receiver.BindNewPipeAndPassRemote());

  mojom::MCPServerPtr server = Add();
  std::vector<mojom::MCPServerPtr> servers = mojom_observer.future.Take();
  ASSERT_EQ(servers.size(), 1u);
  EXPECT_EQ(*servers[0], *server);

  ASSERT_TRUE(
      Update(server->id, Params("Example", kEndpoint, /*enabled=*/false))
          .has_value());
  ASSERT_EQ(mojom_observer.future.Take().size(), 1u);
  EXPECT_EQ(Remove(server->id), std::nullopt);
  EXPECT_TRUE(mojom_observer.future.Take().empty());
}

TEST_F(MCPHostTest, ListsEnabledServers) {
  // Added before this profile's host could decrypt tokens.
  DeferredOSCryptAsync other_os_crypt(encryptor_);
  other_os_crypt.Complete();
  MCPServerRegistry other_registry(prefs_, other_os_crypt);
  mojom::MCPServerPtr enabled =
      other_registry.AddServer(*Params("Enabled"), std::string(kToken)).value();
  ASSERT_TRUE(other_registry
                  .AddServer(*Params("Disabled", kEndpoint, /*enabled=*/false),
                             std::nullopt)
                  .has_value());
  EXPECT_TRUE(host_->GetEnabledServers().empty());
  EXPECT_FALSE(host_->IsReady());

  TestObserver observer(*host_);
  host_->AddObserver(&observer);
  os_crypt_.Complete();
  GetServers();
  EXPECT_TRUE(host_->IsReady());
  std::vector<MCPHost::EnabledServer> servers = host_->GetEnabledServers();
  ASSERT_EQ(servers.size(), 1u);
  EXPECT_EQ(*servers[0].server, *enabled);
  EXPECT_TRUE(servers[0].client);
  EXPECT_EQ(servers[0].client.get(), host_->GetClient(enabled->id).get());
  EXPECT_EQ(observer.enabled_server_counts, std::vector<size_t>({1}));
  host_->RemoveObserver(&observer);
}

TEST_F(MCPHostTest, NotifiesObserversOfClientChanges) {
  TestObserver observer(*host_);
  host_->AddObserver(&observer);
  // With no servers, becoming ready changes nothing.
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  EXPECT_EQ(observer.enabled_server_counts, std::vector<size_t>({1}));

  // Not something a client uses.
  ASSERT_TRUE(Update(server->id, Params("Example", kEndpoint, /*enabled=*/true,
                                        /*auto_attach=*/true))
                  .has_value());
  ASSERT_TRUE(
      Update(server->id, Params("Example", kEndpoint, /*enabled=*/false))
          .has_value());
  ASSERT_TRUE(Update(server->id, Params("Example")).has_value());
  ASSERT_TRUE(Update(server->id, Params("Renamed")).has_value());
  EXPECT_EQ(Remove(server->id), std::nullopt);
  EXPECT_EQ(observer.enabled_server_counts,
            std::vector<size_t>({1, 0, 1, 1, 0}));
  host_->RemoveObserver(&observer);
}

TEST_F(MCPHostTest, GetServerTools) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::test::TestFuture<ToolsResult> future;
  remote_->GetServerTools(server->id, future.GetCallback());
  const size_t list = server_requests_.CompleteHandshake(0);
  server_requests_.WaitForRequest(list);
  ASSERT_EQ(server_requests_.Method(list), "tools/list");
  server_requests_.RespondResult(list, R"({"tools":[
    {"name":"search","title":"Search","description":"Searches",
     "inputSchema":{"type":"object"}},
    {"name":"fetch","inputSchema":{"type":"object"}}
  ]})");
  ToolsResult result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 2u);
  EXPECT_EQ(*(*result)[0],
            *mojom::MCPServerTool::New("search", "Search", "Searches"));
  EXPECT_EQ(*(*result)[1],
            *mojom::MCPServerTool::New("fetch", std::nullopt, std::nullopt));
}

TEST_F(MCPHostTest, GetServerToolsErrors) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server =
      Add(Params("Example", kEndpoint, /*enabled=*/false));
  base::test::TestFuture<ToolsResult> disabled;
  remote_->GetServerTools(server->id, disabled.GetCallback());
  EXPECT_EQ(disabled.Take(),
            base::unexpected(mojom::MCPServerError::kServerDisabled));

  base::test::TestFuture<ToolsResult> unknown;
  remote_->GetServerTools("unknown", unknown.GetCallback());
  EXPECT_EQ(unknown.Take(), base::unexpected(mojom::MCPServerError::kNotFound));

  // Answered even if the server's client goes before the server does.
  ASSERT_TRUE(Update(server->id, Params()).has_value());
  base::test::TestFuture<ToolsResult> replaced;
  remote_->GetServerTools(server->id, replaced.GetCallback());
  server_requests_.WaitForRequest(0);
  ASSERT_TRUE(Update(server->id, Params("Renamed")).has_value());
  EXPECT_EQ(replaced.Take(),
            base::unexpected(mojom::MCPServerError::kServerChanged));

  // The server can't be reached.
  base::test::TestFuture<ToolsResult> failed;
  remote_->GetServerTools(server->id, failed.GetCallback());
  server_requests_.WaitForRequest(1);
  ASSERT_EQ(server_requests_.Method(1), "initialize");
  url_loader_factory_.SimulateResponseWithoutRemovingFromPendingList(
      &url_loader_factory_.pending_requests()->at(1),
      network::mojom::URLResponseHead::New(), "",
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED));
  EXPECT_EQ(failed.Take(),
            base::unexpected(mojom::MCPServerError::kConnectionFailed));
}

TEST_F(MCPHostTest, TokenIsNotSentToAnotherOrigin) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add(Params(), kToken);
  base::test::TestFuture<ToolsResult> before;
  remote_->GetServerTools(server->id, before.GetCallback());
  server_requests_.WaitForRequest(0);
  ASSERT_EQ(server_requests_.Method(0), "initialize");
  EXPECT_EQ(server_requests_.Request(0).headers.GetHeader("Authorization"),
            base::StrCat({"Bearer ", kToken}));

  ServerResult updated =
      Update(server->id, Params("Example", "https://other.example.com/mcp"));
  ASSERT_TRUE(updated.has_value());
  EXPECT_FALSE((*updated)->has_token);
  base::test::TestFuture<ToolsResult> after;
  remote_->GetServerTools(server->id, after.GetCallback());
  server_requests_.WaitForRequest(1);
  const size_t initialize = server_requests_.RequestCount() - 1;
  ASSERT_EQ(server_requests_.Method(initialize), "initialize");
  EXPECT_EQ(server_requests_.Request(initialize).url,
            GURL("https://other.example.com/mcp"));
  EXPECT_FALSE(
      server_requests_.Request(initialize).headers.HasHeader("Authorization"));
}

TEST_F(MCPHostTest, UndecryptableToken) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add(Params(), kToken);
  base::WeakPtr<MCPClient> client = host_->GetClient(server->id);
  ASSERT_TRUE(client);
  TestMojomObserver observer;
  remote_->BindObserver(observer.receiver.BindNewPipeAndPassRemote());
  // Bound by the time this is answered.
  GetServers();

  {
    ScopedDictPrefUpdate update(&prefs_, prefs::kBraveAIChatMCPServers);
    (*update->FindList("servers"))[0].GetDict().Set(
        "token", base::Base64Encode("not a ciphertext"));
  }
  // Not connected without it, and the user is told why.
  EXPECT_FALSE(client);
  EXPECT_FALSE(host_->GetClient(server->id));
  std::vector<mojom::MCPServerPtr> changed = observer.future.Take();
  ASSERT_EQ(changed.size(), 1u);
  EXPECT_TRUE(changed[0]->token_unreadable);
  std::vector<mojom::MCPServerPtr> servers = GetServers();
  ASSERT_EQ(servers.size(), 1u);
  EXPECT_TRUE(servers[0]->has_token);
  EXPECT_TRUE(servers[0]->token_unreadable);

  ServerResult updated = Update(server->id, Params(), kToken);
  ASSERT_TRUE(updated.has_value());
  EXPECT_FALSE((*updated)->token_unreadable);
  EXPECT_TRUE(host_->GetClient(server->id));
}

TEST_F(MCPHostTest, ListingSetsStatus) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);
  EXPECT_FALSE(host_->IsFailing(server->id));

  FailListing(server->id, 0);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);
  EXPECT_TRUE(host_->IsFailing(server->id));

  // The connectors dialog's listing counts too.
  base::test::TestFuture<ToolsResult> tools;
  remote_->GetServerTools(server->id, tools.GetCallback());
  ListTools(1);
  EXPECT_TRUE(tools.Take().has_value());
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);
  EXPECT_FALSE(host_->IsFailing(server->id));
}

// Callers asking at once share one listing, which counts as one failure.
TEST_F(MCPHostTest, ListingsAreShared) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  base::test::TestFuture<MCPClient::ToolsResult> first;
  base::test::TestFuture<MCPClient::ToolsResult> second;
  host_->GetTools(server->id, first.GetCallback());
  host_->GetTools(server->id, second.GetCallback());
  FailInitialize(0);
  EXPECT_FALSE(first.Take().has_value());
  EXPECT_FALSE(second.Take().has_value());
  EXPECT_EQ(server_requests_.RequestCount(), 1u);

  // Retried as after one failure, and then twice as long after.
  task_environment_.FastForwardBy(kMCPToolsRetryDelay);
  ASSERT_EQ(server_requests_.RequestCount(), 2u);
  FailInitialize(1);
  task_environment_.FastForwardBy(kMCPToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), 3u);
}

TEST_F(MCPHostTest, StatusFollowsTheError) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add(Params(), kToken);
  host_->GetTools(server->id, base::DoNothing());
  server_requests_.WaitForRequest(0);
  RespondWithStatus(0, net::HTTP_UNAUTHORIZED);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAuthRequired);

  host_->GetTools(server->id, base::DoNothing());
  server_requests_.WaitForRequest(1);
  RespondWithStatus(1, net::HTTP_SERVICE_UNAVAILABLE);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);

  // A 4xx after the handshake won't pass by trying again.
  host_->GetTools(server->id, base::DoNothing());
  const size_t list = server_requests_.CompleteHandshake(2);
  server_requests_.WaitForRequest(list);
  RespondWithStatus(list, net::HTTP_GONE);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kUnsupported);

  // A token that can't be sent needs replacing.
  std::string ciphertext;
  ASSERT_TRUE(encryptor_->EncryptString("a\nb", &ciphertext));
  {
    ScopedDictPrefUpdate update(&prefs_, prefs::kBraveAIChatMCPServers);
    (*update->FindList("servers"))[0].GetDict().Set(
        "token", base::Base64Encode(ciphertext));
  }
  EXPECT_FALSE(host_->GetClient(server->id));
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAuthRequired);
}

TEST_F(MCPHostTest, StatusesOfServersWithoutClients) {
  os_crypt_.Complete();
  mojom::MCPServerPtr enabled = Add(Params("Enabled"));
  mojom::MCPServerPtr disabled =
      Add(Params("Disabled", kEndpoint, /*enabled=*/false));
  mojom::MCPServerPtr unreadable = Add(Params("Unreadable"), kToken);
  {
    ScopedDictPrefUpdate update(&prefs_, prefs::kBraveAIChatMCPServers);
    (*update->FindList("servers"))[2].GetDict().Set(
        "token", base::Base64Encode("not a ciphertext"));
  }
  EXPECT_EQ(
      GetServerStatuses(),
      (Statuses{{enabled->id, mojom::MCPServerStatus::kAvailable},
                {disabled->id, mojom::MCPServerStatus::kUnavailable},
                {unreadable->id, mojom::MCPServerStatus::kAuthRequired}}));
  EXPECT_EQ(host_->GetServerStatus("unknown"),
            mojom::MCPServerStatus::kUnavailable);
  // There's nothing to wait for.
  EXPECT_FALSE(host_->IsFailing(disabled->id));
  EXPECT_FALSE(host_->IsFailing(unreadable->id));

  EXPECT_EQ(Remove(disabled->id), std::nullopt);
  EXPECT_FALSE(GetServerStatuses().contains(disabled->id));
}

TEST_F(MCPHostTest, StatusChangesAreObserved) {
  os_crypt_.Complete();
  TestObserver observer(*host_);
  host_->AddObserver(&observer);
  TestMojomObserver mojom_observer;
  remote_->BindObserver(mojom_observer.receiver.BindNewPipeAndPassRemote());
  remote_.FlushForTesting();

  mojom::MCPServerPtr server = Add();
  ASSERT_EQ(mojom_observer.future.Take().size(), 1u);
  FailListing(server->id, 0);
  // Unchanged by a second failure.
  FailListing(server->id, 1);
  ASSERT_TRUE(
      Update(server->id, Params("Example", kEndpoint, /*enabled=*/false))
          .has_value());
  ASSERT_EQ(mojom_observer.future.Take().size(), 1u);

  EXPECT_EQ(observer.statuses, (std::vector<mojom::MCPServerStatus>{
                                   mojom::MCPServerStatus::kAvailable,
                                   mojom::MCPServerStatus::kNotResponding,
                                   mojom::MCPServerStatus::kUnavailable}));
  EXPECT_EQ(mojom_observer.pushed_statuses,
            (std::vector<Statuses>{
                {{server->id, mojom::MCPServerStatus::kAvailable}},
                {{server->id, mojom::MCPServerStatus::kNotResponding}},
                {{server->id, mojom::MCPServerStatus::kUnavailable}},
            }));
  host_->RemoveObserver(&observer);
}

// Once a listing has failed, callers don't wait for the server, so it's listed
// again in the background until that works.
TEST_F(MCPHostTest, RetriesWhileInUseUntilListingWorks) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  FailListing(server->id, 0);

  task_environment_.FastForwardBy(kMCPToolsRetryDelay - base::Milliseconds(1));
  EXPECT_EQ(server_requests_.RequestCount(), 1u);
  task_environment_.FastForwardBy(base::Milliseconds(1));
  ASSERT_EQ(server_requests_.RequestCount(), 2u);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);
  ListTools(1);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);

  const size_t request_count = server_requests_.RequestCount();
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), request_count);
}

// A server that lost its session may refuse the old one with something other
// than a 404, so a retry and a reconnect start a new one.
TEST_F(MCPHostTest, RetryAndReconnectStartANewSession) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  host_->GetTools(server->id, base::DoNothing());
  size_t list = server_requests_.CompleteHandshake(0, "session-1");
  server_requests_.WaitForRequest(list);
  server_requests_.RespondError(list, -32600, "Unknown session");
  RunReadyTasks();
  ASSERT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);

  // The retry ends the old session, and starts another.
  task_environment_.FastForwardBy(kMCPToolsRetryDelay);
  ASSERT_EQ(server_requests_.RequestCount(), list + 3);
  EXPECT_EQ(server_requests_.Request(list + 1).method, "DELETE");
  EXPECT_EQ(server_requests_.Method(list + 2), "initialize");
  list = server_requests_.CompleteHandshake(list + 2, "session-2");
  server_requests_.WaitForRequest(list);
  server_requests_.RespondError(list, -32600, "Unknown session");
  RunReadyTasks();

  // As does a reconnect.
  task_environment_.FastForwardBy(kMCPMinReconnectInterval);
  Reconnect(server->id);
  ASSERT_EQ(server_requests_.RequestCount(), list + 3);
  EXPECT_EQ(server_requests_.Request(list + 1).method, "DELETE");
  EXPECT_EQ(server_requests_.Method(list + 2), "initialize");
  ListTools(list + 2);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);
}

TEST_F(MCPHostTest, RetriesLessOftenWhileFailing) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  FailListing(server->id, 0);

  size_t index = 1;
  for (base::TimeDelta delay :
       {base::Minutes(1), base::Minutes(2), base::Minutes(4), base::Minutes(8),
        kMCPMaxToolsRetryDelay, kMCPMaxToolsRetryDelay}) {
    SCOPED_TRACE(delay);
    task_environment_.FastForwardBy(delay - base::Milliseconds(1));
    EXPECT_EQ(server_requests_.RequestCount(), index);
    task_environment_.FastForwardBy(base::Milliseconds(1));
    ASSERT_EQ(server_requests_.RequestCount(), index + 1);
    FailInitialize(index++);
  }
}

TEST_F(MCPHostTest, RetriesOnlyWhileInUse) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  FailListing(server->id, 0);
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), 1u);

  // A retry that came due meanwhile is made once the server's used.
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  base::ScopedClosureRunner other_use = host_->UseServer(server->id);
  RunReadyTasks();
  ASSERT_EQ(server_requests_.RequestCount(), 2u);
  FailInitialize(1);

  // Retried while anything still uses it.
  use.RunAndReset();
  task_environment_.FastForwardBy(base::Minutes(2));
  ASSERT_EQ(server_requests_.RequestCount(), 3u);
  FailInitialize(2);
  other_use.RunAndReset();
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), 3u);
}

TEST_F(MCPHostTest, NoRetryOnceDisabledOrRemoved) {
  os_crypt_.Complete();
  mojom::MCPServerPtr disabled = Add(Params("Disabled"));
  mojom::MCPServerPtr removed = Add(Params("Removed"));
  base::ScopedClosureRunner disabled_use = host_->UseServer(disabled->id);
  base::ScopedClosureRunner removed_use = host_->UseServer(removed->id);
  FailListing(disabled->id, 0);
  FailListing(removed->id, 1);

  ASSERT_TRUE(
      Update(disabled->id, Params("Disabled", kEndpoint, /*enabled=*/false))
          .has_value());
  EXPECT_EQ(Remove(removed->id), std::nullopt);
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), 2u);

  // Turned back on, it starts afresh.
  ASSERT_TRUE(Update(disabled->id, Params("Disabled")).has_value());
  EXPECT_EQ(host_->GetServerStatus(disabled->id),
            mojom::MCPServerStatus::kAvailable);
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), 2u);
}

TEST_F(MCPHostTest, NoRetryAfterShutdown) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  FailListing(server->id, 0);

  host_.reset();
  task_environment_.FastForwardBy(kMCPMaxToolsRetryDelay * 2);
  EXPECT_EQ(server_requests_.RequestCount(), 1u);
}

TEST_F(MCPHostTest, MarkNotRespondingUntilTheListingEnds) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::test::TestFuture<MCPClient::ToolsResult> listed;
  host_->GetTools(server->id, listed.GetCallback());
  server_requests_.WaitForRequest(0);

  host_->MarkNotResponding(server->id);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);
  EXPECT_TRUE(host_->IsFailing(server->id));

  ListTools(0);
  EXPECT_TRUE(listed.Take().has_value());
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);
}

TEST_F(MCPHostTest, ReconnectListsAgainNow) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::ScopedClosureRunner use = host_->UseServer(server->id);
  TestMojomObserver mojom_observer;
  remote_->BindObserver(mojom_observer.receiver.BindNewPipeAndPassRemote());
  remote_.FlushForTesting();
  FailListing(server->id, 0);
  // The retry fails too, so the next waits longer.
  task_environment_.FastForwardBy(kMCPToolsRetryDelay);
  ASSERT_EQ(server_requests_.RequestCount(), 2u);
  FailInitialize(1);

  task_environment_.FastForwardBy(kMCPMinReconnectInterval);
  Reconnect(server->id);
  ASSERT_EQ(server_requests_.RequestCount(), 3u);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kChecking);
  // Not waited for, as it's not known to fail.
  EXPECT_FALSE(host_->IsFailing(server->id));
  FailInitialize(2);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);

  // Retried as soon as after the first failure.
  task_environment_.FastForwardBy(kMCPToolsRetryDelay - base::Milliseconds(1));
  EXPECT_EQ(server_requests_.RequestCount(), 3u);
  task_environment_.FastForwardBy(base::Milliseconds(1));
  ASSERT_EQ(server_requests_.RequestCount(), 4u);
  FailInitialize(3);

  task_environment_.FastForwardBy(kMCPMinReconnectInterval);
  Reconnect(server->id);
  ASSERT_EQ(server_requests_.RequestCount(), 5u);
  ListTools(4);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);
  mojom_observer.receiver.FlushForTesting();
  std::vector<mojom::MCPServerStatus> pushed;
  for (const Statuses& statuses : mojom_observer.pushed_statuses) {
    pushed.push_back(statuses.at(server->id));
  }
  EXPECT_EQ(pushed, (std::vector<mojom::MCPServerStatus>{
                        mojom::MCPServerStatus::kNotResponding,
                        mojom::MCPServerStatus::kChecking,
                        mojom::MCPServerStatus::kNotResponding,
                        mojom::MCPServerStatus::kChecking,
                        mojom::MCPServerStatus::kAvailable}));
}

TEST_F(MCPHostTest, ReconnectOnlyWhileFailing) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  Reconnect(server->id);
  Reconnect("unknown");
  RunReadyTasks();
  EXPECT_EQ(server_requests_.RequestCount(), 0u);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);
}

// The page can ask to reconnect as often as it likes, but the server isn't
// asked more than every few seconds, nor twice at once.
TEST_F(MCPHostTest, ReconnectIsThrottled) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  FailListing(server->id, 0);
  Reconnect(server->id);
  RunReadyTasks();
  EXPECT_EQ(server_requests_.RequestCount(), 1u);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kNotResponding);

  // A listing under way is joined, in its session.
  task_environment_.FastForwardBy(kMCPMinReconnectInterval);
  host_->GetTools(server->id, base::DoNothing());
  const size_t list = server_requests_.CompleteHandshake(1, "session-1");
  server_requests_.WaitForRequest(list);
  ASSERT_EQ(server_requests_.Method(list), "tools/list");
  task_environment_.FastForwardBy(kMCPMinReconnectInterval);
  Reconnect(server->id);
  Reconnect(server->id);
  RunReadyTasks();
  EXPECT_EQ(server_requests_.RequestCount(), list + 1);
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kChecking);
  server_requests_.RespondResult(list, R"({"tools":[]})");
  RunReadyTasks();
  EXPECT_EQ(host_->GetServerStatus(server->id),
            mojom::MCPServerStatus::kAvailable);

  // The session is kept.
  task_environment_.FastForwardBy(kMCPToolsCacheTtl);
  host_->GetTools(server->id, base::DoNothing());
  server_requests_.WaitForRequest(list + 1);
  EXPECT_EQ(server_requests_.Method(list + 1), "tools/list");
  EXPECT_EQ(
      server_requests_.Request(list + 1).headers.GetHeader("Mcp-Session-Id"),
      "session-1");
}

TEST_F(MCPHostTest, DestroyingTheHostDestroysClients) {
  os_crypt_.Complete();
  mojom::MCPServerPtr server = Add();
  base::WeakPtr<MCPClient> client = host_->GetClient(server->id);
  ASSERT_TRUE(client);
  host_.reset();
  EXPECT_FALSE(client);
  remote_.FlushForTesting();
  EXPECT_FALSE(remote_.is_connected());
}

}  // namespace ai_chat
