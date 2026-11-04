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
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
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

  std::vector<size_t> enabled_server_counts;

 private:
  raw_ref<MCPHost> host_;
};

class TestMojomObserver : public mojom::MCPHostObserver {
 public:
  void OnServersChanged(std::vector<mojom::MCPServerPtr> servers) override {
    future.SetValue(std::move(servers));
  }

  base::test::TestFuture<std::vector<mojom::MCPServerPtr>> future;
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
