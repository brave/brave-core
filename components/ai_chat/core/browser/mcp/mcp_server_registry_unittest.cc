/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_server_registry.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/mcp/test/deferred_os_crypt_async.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::test::ParseJsonDict;

namespace ai_chat {

namespace {

constexpr char kEndpoint[] = "https://mcp.example.com/mcp";
constexpr char kToken[] = "secret-token-1234";

mojom::MCPServerParams Params(std::string_view name = "Example",
                              std::string_view endpoint = kEndpoint,
                              bool enabled = true,
                              bool auto_attach = false) {
  return mojom::MCPServerParams(std::string(name), GURL(endpoint), enabled,
                                auto_attach);
}

class TestObserver : public MCPServerRegistry::Observer {
 public:
  void OnMCPServersChanged() override { ++changes; }

  int changes = 0;
};

}  // namespace

class MCPServerRegistryTest : public testing::Test {
 protected:
  MCPServerRegistryTest() {
    prefs::RegisterProfilePrefs(prefs_.registry());
    os_crypt_.Complete();
    registry_ = CreateRegistry();
  }

  std::unique_ptr<MCPServerRegistry> CreateRegistry() {
    auto registry = std::make_unique<MCPServerRegistry>(prefs_, os_crypt_);
    EXPECT_TRUE(registry->ready().is_signaled());
    return registry;
  }

  mojom::MCPServerPtr Add(mojom::MCPServerParams params = Params(),
                          std::optional<std::string> token = std::nullopt) {
    MCPServerRegistry::Result result =
        registry_->AddServer(params, std::move(token));
    EXPECT_TRUE(result.has_value());
    return result.has_value() ? std::move(*result) : nullptr;
  }

  const base::DictValue& Pref() {
    return prefs_.GetDict(prefs::kBraveAIChatMCPServers);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_ =
      os_crypt_async::GetTestEncryptorForTesting();
  DeferredOSCryptAsync os_crypt_{encryptor_};
  std::unique_ptr<MCPServerRegistry> registry_;
};

TEST_F(MCPServerRegistryTest, AddsServers) {
  EXPECT_TRUE(registry_->GetServers().empty());
  mojom::MCPServerPtr first = Add(Params("First"));
  mojom::MCPServerPtr second =
      Add(Params("Second", "http://localhost:3000/mcp", /*enabled=*/false,
                 /*auto_attach=*/true),
          kToken);
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  EXPECT_TRUE(base::Uuid::ParseLowercase(first->id).is_valid());
  EXPECT_NE(first->id, second->id);
  EXPECT_EQ(*second, *mojom::MCPServer::New(second->id, "Second",
                                            GURL("http://localhost:3000/mcp"),
                                            /*enabled=*/false,
                                            /*auto_attach=*/true, "second",
                                            /*has_token=*/true,
                                            /*token_unreadable=*/false));

  std::vector<mojom::MCPServerPtr> servers = registry_->GetServers();
  ASSERT_EQ(servers.size(), 2u);
  EXPECT_EQ(*servers[0], *first);
  EXPECT_EQ(*servers[1], *second);
  EXPECT_EQ(*registry_->GetServer(first->id), *first);
  EXPECT_FALSE(registry_->GetServer("unknown"));
}

TEST_F(MCPServerRegistryTest, PrefLayout) {
  mojom::MCPServerPtr server = Add(Params("Example"));
  EXPECT_EQ(Pref(), ParseJsonDict(R"({"version": 1, "servers": [{
              "id": ")" + server->id +
                                  R"(",
              "name": "Example",
              "slug": "example",
              "endpoint": "https://mcp.example.com/mcp",
              "transport": "streamable_http",
              "enabled": true,
              "auto_attach": false
            }]})"));
}

TEST_F(MCPServerRegistryTest, PrefRoundTrip) {
  Add(Params("First"), kToken);
  Add(Params("Second", "http://127.0.0.1:8080/mcp", /*enabled=*/false,
             /*auto_attach=*/true));
  std::vector<mojom::MCPServerPtr> servers = registry_->GetServers();

  registry_ = CreateRegistry();
  EXPECT_EQ(registry_->GetServers(), servers);
  std::optional<MCPServerConfig> config =
      registry_->GetClientConfig(servers[0]->id);
  ASSERT_TRUE(config);
  EXPECT_EQ(*config,
            MCPServerConfig(GURL(kEndpoint), std::string(kToken), "First"));
  config = registry_->GetClientConfig(servers[1]->id);
  ASSERT_TRUE(config);
  EXPECT_EQ(*config, MCPServerConfig(GURL("http://127.0.0.1:8080/mcp"),
                                     std::nullopt, "Second"));
  EXPECT_FALSE(registry_->GetClientConfig("unknown"));
}

TEST_F(MCPServerRegistryTest, TokenIsEncrypted) {
  mojom::MCPServerPtr server = Add(Params(), kToken);
  EXPECT_TRUE(server->has_token);

  const std::string raw_pref = *base::WriteJson(Pref());
  EXPECT_EQ(raw_pref.find(kToken), std::string::npos) << raw_pref;
  const std::string* stored =
      (*Pref().FindList("servers"))[0].GetDict().FindString("token");
  ASSERT_TRUE(stored);
  std::optional<std::vector<uint8_t>> ciphertext = base::Base64Decode(*stored);
  ASSERT_TRUE(ciphertext);
  std::optional<std::string> plaintext = encryptor_->DecryptData(*ciphertext);
  EXPECT_EQ(plaintext, kToken);
  EXPECT_EQ(registry_->GetClientConfig(server->id)->bearer_token, kToken);
}

TEST_F(MCPServerRegistryTest, UndecryptableToken) {
  mojom::MCPServerPtr server = Add(Params(), kToken);
  {
    ScopedDictPrefUpdate update(&prefs_, prefs::kBraveAIChatMCPServers);
    (*update->FindList("servers"))[0].GetDict().Set(
        "token", base::Base64Encode("not a ciphertext"));
  }
  // Still listed, saying so, so that the user can enter the token again.
  mojom::MCPServerPtr listed = registry_->GetServer(server->id);
  EXPECT_TRUE(listed->has_token);
  EXPECT_TRUE(listed->token_unreadable);
  EXPECT_TRUE(registry_->GetServers()[0]->token_unreadable);
  EXPECT_FALSE(registry_->GetClientConfig(server->id));

  MCPServerRegistry::Result result =
      registry_->UpdateServer(server->id, Params(), std::nullopt);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE((*result)->token_unreadable);
  result = registry_->UpdateServer(server->id, Params(), kToken);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE((*result)->token_unreadable);
  EXPECT_FALSE(registry_->GetServer(server->id)->token_unreadable);
  EXPECT_TRUE(registry_->GetClientConfig(server->id));
}

TEST_F(MCPServerRegistryTest, ValidatesEndpoint) {
  mojom::MCPServerPtr server = Add();
  const base::DictValue pref = Pref().Clone();
  for (const auto& [endpoint, error] :
       std::vector<std::pair<std::string, mojom::MCPServerError>>{
           {"not a url", mojom::MCPServerError::kInvalidUrl},
           {"https://0.0.0.0/mcp", mojom::MCPServerError::kInvalidUrl},
           {"ftp://mcp.example.com/",
            mojom::MCPServerError::kUnsupportedScheme},
           {"https://user:pass@mcp.example.com/",
            mojom::MCPServerError::kEmbeddedCredentials},
           {"http://mcp.example.com/mcp",
            mojom::MCPServerError::kInsecureScheme},
           {"https://192.168.1.10/mcp",
            mojom::MCPServerError::kPrivateAddressNotAllowed},
           {"https://mcp.example.com/" +
                std::string(kMaxMCPServerEndpointLength, 'a'),
            mojom::MCPServerError::kEndpointTooLong},
       }) {
    SCOPED_TRACE(endpoint);
    EXPECT_EQ(registry_->AddServer(Params("Name", endpoint), std::nullopt),
              base::unexpected(error));
    EXPECT_EQ(
        registry_->UpdateServer(server->id, Params("Name", endpoint), kToken),
        base::unexpected(error));
  }
  // Nothing was saved.
  EXPECT_EQ(Pref(), pref);

  const std::string longest =
      "https://mcp.example.com/" +
      std::string(kMaxMCPServerEndpointLength -
                      std::string_view("https://mcp.example.com/").size(),
                  'a');
  for (const std::string& endpoint : {std::string("http://localhost:3000/mcp"),
                                      std::string("http://127.0.0.1/mcp"),
                                      std::string("http://[::1]/"), longest}) {
    SCOPED_TRACE(endpoint);
    EXPECT_TRUE(registry_->AddServer(Params("Local", endpoint), std::nullopt)
                    .has_value());
  }
}

TEST_F(MCPServerRegistryTest, ValidatesName) {
  for (const std::string& name : {std::string(), std::string("  \n"),
                                  std::string("a\x01"
                                              "b"),
                                  std::string(kMaxMCPServerNameLength + 1, 'a'),
                                  std::string("\xFF\xFE")}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(registry_->AddServer(Params(name), std::nullopt),
              base::unexpected(mojom::MCPServerError::kInvalidName));
  }
  EXPECT_TRUE(registry_->GetServers().empty());

  EXPECT_EQ(Add(Params("  Spaced Out \n"))->name, "Spaced Out");
  EXPECT_TRUE(Add(Params(std::string(kMaxMCPServerNameLength, 'a'))));
}

TEST_F(MCPServerRegistryTest, ValidatesToken) {
  for (const std::string& token :
       {std::string(), std::string(" \t"), std::string("a\nb"),
        std::string("a\rb"), std::string(kMaxMCPServerTokenLength + 1, 't')}) {
    SCOPED_TRACE(token);
    EXPECT_EQ(registry_->AddServer(Params(), token),
              base::unexpected(mojom::MCPServerError::kInvalidToken));
  }
  EXPECT_TRUE(registry_->GetServers().empty());

  mojom::MCPServerPtr server = Add(Params(), "  token \n");
  EXPECT_EQ(registry_->GetClientConfig(server->id)->bearer_token, "token");
}

TEST_F(MCPServerRegistryTest, UpdatesServer) {
  mojom::MCPServerPtr server = Add(Params("Old"), kToken);
  MCPServerRegistry::Result result = registry_->UpdateServer(
      server->id,
      Params("New", "https://mcp.example.com/v2", /*enabled=*/false,
             /*auto_attach=*/true),
      std::nullopt);
  ASSERT_TRUE(result.has_value());
  // The slug doesn't change, and nor, at the same origin, does the token
  // unless one is given.
  EXPECT_EQ(**result, *mojom::MCPServer::New(
                          server->id, "New", GURL("https://mcp.example.com/v2"),
                          /*enabled=*/false, /*auto_attach=*/true, "old",
                          /*has_token=*/true, /*token_unreadable=*/false));
  EXPECT_EQ(*registry_->GetServer(server->id), **result);
  EXPECT_EQ(registry_->GetClientConfig(server->id),
            MCPServerConfig(GURL("https://mcp.example.com/v2"),
                            std::string(kToken), "New"));

  ASSERT_TRUE(registry_->UpdateServer(server->id, Params("New"), "replacement")
                  .has_value());
  EXPECT_EQ(registry_->GetClientConfig(server->id)->bearer_token,
            "replacement");

  result = registry_->ClearServerToken(server->id);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE((*result)->has_token);
  EXPECT_FALSE(registry_->GetServer(server->id)->has_token);
  EXPECT_EQ(registry_->GetClientConfig(server->id)->bearer_token, std::nullopt);
  EXPECT_FALSE((*Pref().FindList("servers"))[0].GetDict().contains("token"));
}

TEST_F(MCPServerRegistryTest, TokenIsDroppedForAnotherOrigin) {
  for (const char* endpoint :
       {"https://other.example.com/mcp", "https://mcp.example.com:8443/mcp",
        "http://localhost/mcp"}) {
    SCOPED_TRACE(endpoint);
    mojom::MCPServerPtr server = Add(Params(), kToken);
    // The token isn't for that origin, so it mustn't be sent there.
    MCPServerRegistry::Result result = registry_->UpdateServer(
        server->id, Params("Example", endpoint), std::nullopt);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE((*result)->has_token);
    EXPECT_FALSE(registry_->GetServer(server->id)->has_token);
    EXPECT_EQ(registry_->GetClientConfig(server->id),
              MCPServerConfig(GURL(endpoint), std::nullopt, "Example"));
    EXPECT_EQ(registry_->RemoveServer(server->id), std::nullopt);
  }

  // Unless a token is given with the change.
  mojom::MCPServerPtr server = Add(Params(), kToken);
  ASSERT_TRUE(
      registry_
          ->UpdateServer(server->id,
                         Params("Example", "https://other.example.com/"),
                         "other-token")
          .has_value());
  EXPECT_EQ(registry_->GetClientConfig(server->id)->bearer_token,
            "other-token");
}

TEST_F(MCPServerRegistryTest, LimitsServers) {
  for (size_t i = 0; i < kMaxMCPServers; ++i) {
    ASSERT_TRUE(Add());
  }
  const base::DictValue pref = Pref().Clone();
  EXPECT_EQ(registry_->AddServer(Params(), std::nullopt),
            base::unexpected(mojom::MCPServerError::kTooManyServers));
  EXPECT_EQ(Pref(), pref);

  EXPECT_EQ(registry_->RemoveServer(registry_->GetServers()[0]->id),
            std::nullopt);
  EXPECT_TRUE(Add());
}

TEST_F(MCPServerRegistryTest, RemovesServer) {
  mojom::MCPServerPtr first = Add(Params("First"));
  mojom::MCPServerPtr second = Add(Params("Second"));
  EXPECT_EQ(registry_->RemoveServer(first->id), std::nullopt);
  EXPECT_EQ(registry_->RemoveServer(first->id),
            mojom::MCPServerError::kNotFound);
  std::vector<mojom::MCPServerPtr> servers = registry_->GetServers();
  ASSERT_EQ(servers.size(), 1u);
  EXPECT_EQ(*servers[0], *second);
}

TEST_F(MCPServerRegistryTest, UnknownServer) {
  EXPECT_EQ(registry_->UpdateServer("unknown", Params(), std::nullopt),
            base::unexpected(mojom::MCPServerError::kNotFound));
  EXPECT_EQ(registry_->ClearServerToken("unknown"),
            base::unexpected(mojom::MCPServerError::kNotFound));
  EXPECT_EQ(registry_->RemoveServer("unknown"),
            mojom::MCPServerError::kNotFound);
  EXPECT_TRUE(Pref().empty());
}

TEST_F(MCPServerRegistryTest, Slugs) {
  EXPECT_EQ(Add(Params("Exa"))->slug, "exa");
  EXPECT_EQ(Add(Params("EXA"))->slug, "exa2");
  EXPECT_EQ(Add(Params("exa"))->slug, "exa3");
  EXPECT_EQ(Add(Params("GitHub MCP_Server!"))->slug, "githubmcpserver");
  EXPECT_EQ(Add(Params("\xE6\x97\xA5\xE6\x9C\xAC"))->slug, "server");
  EXPECT_EQ(Add(Params("--"))->slug, "server2");
  EXPECT_EQ(Add(Params("A very long server name"))->slug, "averylongservern");
  EXPECT_EQ(Add(Params("A very long server name"))->slug, "averylongserver2");

  // It's fixed when the server is added, and only reused once the server is
  // gone.
  mojom::MCPServerPtr server = Add(Params("Renamed"));
  ASSERT_TRUE(registry_->UpdateServer(server->id, Params("Exa"), std::nullopt)
                  .has_value());
  EXPECT_EQ(registry_->GetServer(server->id)->slug, "renamed");
  EXPECT_EQ(registry_->RemoveServer(server->id), std::nullopt);
  EXPECT_EQ(Add(Params("Renamed"))->slug, "renamed");
}

TEST_F(MCPServerRegistryTest, KeepsServersOfAnotherVersion) {
  const std::string server = R"({
    "id": "a", "name": "A", "slug": "a", "endpoint": "https://a.example.com/",
    "transport": "streamable_http", "enabled": true, "auto_attach": false
  })";
  // Saved by a newer version of the browser, or a corrupted one.
  for (const char* version : {"2", R"("1")"}) {
    SCOPED_TRACE(version);
    const base::DictValue pref = ParseJsonDict(
        base::StrCat({R"({"version": )", version, R"(, "servers": [)", server,
                      R"(], "future": true})"}));
    prefs_.SetDict(prefs::kBraveAIChatMCPServers, pref.Clone());
    EXPECT_TRUE(registry_->GetServers().empty());
    EXPECT_FALSE(registry_->GetServer("a"));
    EXPECT_FALSE(registry_->GetClientConfig("a"));

    // Left intact for the version that saved them.
    constexpr auto kError = mojom::MCPServerError::kUnsupportedSettingsVersion;
    EXPECT_EQ(registry_->AddServer(Params(), std::nullopt),
              base::unexpected(kError));
    EXPECT_EQ(registry_->UpdateServer("a", Params(), std::nullopt),
              base::unexpected(kError));
    EXPECT_EQ(registry_->ClearServerToken("a"), base::unexpected(kError));
    EXPECT_EQ(registry_->RemoveServer("a"), kError);
    EXPECT_EQ(Pref(), pref);
  }
}

TEST_F(MCPServerRegistryTest, IgnoresMalformedServers) {
  prefs_.SetDict(prefs::kBraveAIChatMCPServers, ParseJsonDict(R"({
    "version": 1,
    "servers": [
      "server",
      {"id": "a", "name": "A", "slug": "a", "endpoint": "https://a.example/",
       "transport": "streamable_http", "enabled": true, "auto_attach": false},
      {"id": "b", "name": "B", "slug": "b", "endpoint": "https://b.example/",
       "transport": "stdio", "enabled": true, "auto_attach": false},
      {"id": "c", "name": "C", "slug": "C_1", "endpoint": "https://c.example/",
       "transport": "streamable_http", "enabled": true, "auto_attach": false},
      {"id": "d", "name": "D", "slug": "d", "endpoint": "not a url",
       "transport": "streamable_http", "enabled": true, "auto_attach": false},
      {"id": "e", "name": "E", "slug": "e", "endpoint": "https://e.example/",
       "transport": "streamable_http", "enabled": true},
      {"id": "a", "name": "Same id", "slug": "f",
       "endpoint": "https://f.example/", "transport": "streamable_http",
       "enabled": true, "auto_attach": false},
      {"id": "g", "name": "Same slug", "slug": "a",
       "endpoint": "https://g.example/", "transport": "streamable_http",
       "enabled": true, "auto_attach": false},
      {"id": "h", "name": "H", "slug": "h", "endpoint": "https://h.example/",
       "transport": "streamable_http", "enabled": false, "auto_attach": true}
    ]
  })"));
  std::vector<mojom::MCPServerPtr> servers = registry_->GetServers();
  ASSERT_EQ(servers.size(), 2u);
  EXPECT_EQ(servers[0]->id, "a");
  EXPECT_EQ(servers[0]->name, "A");
  EXPECT_EQ(servers[1]->id, "h");

  // Changes reach the server that's listed.
  ASSERT_TRUE(registry_->UpdateServer("h", Params("Changed"), std::nullopt)
                  .has_value());
  EXPECT_EQ(*(*Pref().FindList("servers"))[8].GetDict().FindString("name"),
            "Changed");
  EXPECT_EQ(registry_->RemoveServer("a"), std::nullopt);
  EXPECT_EQ(*(*Pref().FindList("servers"))[1].GetDict().FindString("id"), "b");
}

TEST_F(MCPServerRegistryTest, NotifiesObservers) {
  TestObserver observer;
  registry_->AddObserver(&observer);
  mojom::MCPServerPtr server = Add();
  EXPECT_EQ(observer.changes, 1);
  ASSERT_TRUE(registry_->UpdateServer(server->id, Params("New"), std::nullopt)
                  .has_value());
  EXPECT_EQ(observer.changes, 2);
  ASSERT_TRUE(registry_->ClearServerToken(server->id).has_value());
  EXPECT_EQ(observer.changes, 3);
  EXPECT_EQ(registry_->RemoveServer(server->id), std::nullopt);
  EXPECT_EQ(observer.changes, 4);

  // Failures change nothing.
  EXPECT_FALSE(registry_->AddServer(Params(""), std::nullopt).has_value());
  EXPECT_EQ(registry_->RemoveServer(server->id),
            mojom::MCPServerError::kNotFound);
  EXPECT_EQ(observer.changes, 4);
  registry_->RemoveObserver(&observer);
}

TEST_F(MCPServerRegistryTest, TokensNeedTheEncryptor) {
  mojom::MCPServerPtr server = Add(Params(), kToken);

  DeferredOSCryptAsync deferred_os_crypt(encryptor_);
  MCPServerRegistry registry(prefs_, deferred_os_crypt);
  EXPECT_FALSE(registry.ready().is_signaled());
  EXPECT_EQ(registry.GetServers().size(), 1u);
  EXPECT_FALSE(registry.GetClientConfig(server->id));
  EXPECT_EQ(registry.AddServer(Params(), kToken),
            base::unexpected(mojom::MCPServerError::kTokenEncryptionFailed));
  EXPECT_TRUE(registry.AddServer(Params(), std::nullopt).has_value());

  base::test::TestFuture<void> ready;
  registry.ready().Post(FROM_HERE, ready.GetCallback());
  deferred_os_crypt.Complete();
  EXPECT_TRUE(ready.Wait());
  EXPECT_EQ(registry.GetClientConfig(server->id)->bearer_token, kToken);
  EXPECT_TRUE(registry.AddServer(Params(), kToken).has_value());
}

TEST_F(MCPServerRegistryTest, TokenEncryptionFailure) {
  DeferredOSCryptAsync os_crypt(
      os_crypt_async::GetTestEncryptorWithoutKeysForTesting());
  os_crypt.Complete();
  MCPServerRegistry registry(prefs_, os_crypt);
  EXPECT_EQ(registry.AddServer(Params(), kToken),
            base::unexpected(mojom::MCPServerError::kTokenEncryptionFailed));
  EXPECT_TRUE(registry.GetServers().empty());
}

}  // namespace ai_chat
