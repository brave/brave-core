/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_host.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/clone_traits.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

using ToolsResult = base::expected<std::vector<MCPToolDefinition>, MCPError>;
using ServerToolsResult =
    base::expected<std::vector<mojom::MCPServerToolPtr>, mojom::MCPServerError>;

void OnServerTools(mojom::MCPHost::GetServerToolsCallback callback,
                   ToolsResult result) {
  if (!result.has_value()) {
    std::move(callback).Run(
        base::unexpected(mojom::MCPServerError::kConnectionFailed));
    return;
  }
  std::vector<mojom::MCPServerToolPtr> tools;
  tools.reserve(result->size());
  for (MCPToolDefinition& tool : *result) {
    tools.push_back(mojom::MCPServerTool::New(std::move(tool.name),
                                              std::move(tool.title),
                                              std::move(tool.description)));
  }
  std::move(callback).Run(std::move(tools));
}

}  // namespace

MCPHost::Client::Client(MCPServerConfig config,
                        std::unique_ptr<MCPClient> client)
    : config(std::move(config)), client(std::move(client)) {}
MCPHost::Client::Client(Client&&) noexcept = default;
MCPHost::Client& MCPHost::Client::operator=(Client&&) noexcept = default;
MCPHost::Client::~Client() = default;

MCPHost::MCPHost(
    PrefService& prefs,
    os_crypt_async::OSCryptAsync& os_crypt_async,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)),
      registry_(prefs, os_crypt_async) {
  registry_observation_.Observe(&registry_);
  registry_.ready().Post(FROM_HERE,
                         base::BindOnce(&MCPHost::OnRegistryReady,
                                        weak_ptr_factory_.GetWeakPtr()));
}

MCPHost::~MCPHost() = default;

void MCPHost::Bind(mojo::PendingReceiver<mojom::MCPHost> receiver) {
  registry_.ready().Post(
      FROM_HERE,
      base::BindOnce(&MCPHost::BindReady, weak_ptr_factory_.GetWeakPtr(),
                     std::move(receiver)));
}

base::WeakPtr<MCPClient> MCPHost::GetClientForTesting(
    std::string_view server_id) const {
  auto it = clients_.find(server_id);
  if (it == clients_.end()) {
    return nullptr;
  }
  return it->second.client->GetWeakPtr();
}

void MCPHost::GetServers(GetServersCallback callback) {
  std::move(callback).Run(registry_.GetServers());
}

void MCPHost::AddServer(mojom::MCPServerParamsPtr params,
                        const std::optional<std::string>& token,
                        AddServerCallback callback) {
  std::move(callback).Run(registry_.AddServer(*params, token));
}

void MCPHost::UpdateServer(const std::string& id,
                           mojom::MCPServerParamsPtr params,
                           const std::optional<std::string>& token,
                           UpdateServerCallback callback) {
  std::move(callback).Run(registry_.UpdateServer(id, *params, token));
}

void MCPHost::ClearServerToken(const std::string& id,
                               ClearServerTokenCallback callback) {
  std::move(callback).Run(registry_.ClearServerToken(id));
}

void MCPHost::RemoveServer(const std::string& id,
                           RemoveServerCallback callback) {
  std::move(callback).Run(registry_.RemoveServer(id));
}

void MCPHost::GetServerTools(const std::string& id,
                             GetServerToolsCallback callback) {
  auto it = clients_.find(id);
  if (it == clients_.end()) {
    mojom::MCPServerError error = mojom::MCPServerError::kTokenUnreadable;
    if (mojom::MCPServerPtr server = registry_.GetServer(id); !server) {
      error = mojom::MCPServerError::kNotFound;
    } else if (!server->enabled) {
      error = mojom::MCPServerError::kServerDisabled;
    }
    std::move(callback).Run(base::unexpected(error));
    return;
  }
  // The client drops the callback if it's replaced or destroyed first.
  it->second.client->GetTools(base::BindOnce(
      &OnServerTools,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          std::move(callback), ServerToolsResult(base::unexpected(
                                   mojom::MCPServerError::kServerChanged)))));
}

void MCPHost::BindObserver(
    mojo::PendingRemote<mojom::MCPHostObserver> observer) {
  remote_observers_.Add(std::move(observer));
}

void MCPHost::OnMCPServersChanged() {
  if (!registry_.ready().is_signaled()) {
    return;
  }
  // Read once, as reading decrypts each token.
  const std::vector<mojom::MCPServerPtr> servers = registry_.GetServers();
  UpdateClients(servers);
  for (auto& observer : remote_observers_) {
    observer->OnServersChanged(mojo::Clone(servers));
  }
}

void MCPHost::OnRegistryReady() {
  UpdateClients(registry_.GetServers());
}

void MCPHost::BindReady(mojo::PendingReceiver<mojom::MCPHost> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void MCPHost::UpdateClients(const std::vector<mojom::MCPServerPtr>& servers) {
  std::map<std::string, Client, std::less<>> clients;
  for (const mojom::MCPServerPtr& server : servers) {
    if (!server->enabled || server->token_unreadable) {
      continue;
    }
    std::optional<MCPServerConfig> config =
        registry_.GetClientConfig(server->id);
    if (!config) {
      continue;
    }
    if (auto it = clients_.find(server->id);
        it != clients_.end() && it->second.config == *config) {
      clients.insert(clients_.extract(it));
      continue;
    }
    // The registry only holds configs that are valid.
    base::expected<std::unique_ptr<MCPClient>, MCPError> client =
        MCPClient::Create(*config, url_loader_factory_);
    if (!client.has_value()) {
      continue;
    }
    clients.emplace(server->id, Client(std::move(*config), std::move(*client)));
  }
  // The clients of servers that were removed, disabled or changed are
  // destroyed after the swap, so that anything they run as they go sees the
  // current ones.
  std::swap(clients_, clients);
  clients.clear();
}

}  // namespace ai_chat
