/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_host.h"

#include <utility>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/clone_traits.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

using ServerToolsResult =
    base::expected<std::vector<mojom::MCPServerToolPtr>, mojom::MCPServerError>;

MCPClient::ToolsResult CloneResult(const MCPClient::ToolsResult& result) {
  if (!result.has_value()) {
    return base::unexpected(result.error());
  }
  return base::ToVector(*result, &MCPToolDefinition::Clone);
}

void OnServerTools(mojom::MCPHost::GetServerToolsCallback callback,
                   MCPClient::ToolsResult result) {
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

MCPHost::EnabledServer::EnabledServer() = default;
MCPHost::EnabledServer::EnabledServer(EnabledServer&&) noexcept = default;
MCPHost::EnabledServer& MCPHost::EnabledServer::operator=(
    EnabledServer&&) noexcept = default;
MCPHost::EnabledServer::~EnabledServer() = default;

MCPHost::Client::Client(MCPServerConfig config,
                        std::unique_ptr<MCPClient> client,
                        base::RepeatingClosure retry)
    : config(std::move(config)),
      client(std::move(client)),
      health(std::move(retry)) {}
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

std::vector<MCPHost::EnabledServer> MCPHost::GetEnabledServers() const {
  std::vector<EnabledServer> enabled_servers;
  for (mojom::MCPServerPtr& server : registry_.GetServers()) {
    base::WeakPtr<MCPClient> client = GetClient(server->id);
    if (!client) {
      continue;
    }
    EnabledServer& enabled_server = enabled_servers.emplace_back();
    enabled_server.server = std::move(server);
    enabled_server.client = std::move(client);
  }
  return enabled_servers;
}

base::WeakPtr<MCPClient> MCPHost::GetClient(std::string_view server_id) const {
  auto it = clients_.find(server_id);
  if (it == clients_.end()) {
    return nullptr;
  }
  return it->second.client->GetWeakPtr();
}

bool MCPHost::IsReady() const {
  return clients_ready_.is_signaled();
}

void MCPHost::RunWhenReady(base::OnceClosure callback) {
  clients_ready_.Post(FROM_HERE, std::move(callback));
}

void MCPHost::GetTools(std::string_view server_id,
                       MCPClient::GetToolsCallback callback) {
  Client* client = FindClient(server_id);
  if (!client) {
    return;
  }
  client->listing_callbacks.push_back(std::move(callback));
  if (client->listing_callbacks.size() > 1) {
    return;
  }
  client->health.OnListingStarted();
  // The client drops the callback if it's destroyed first.
  client->client->GetTools(base::BindOnce(&MCPHost::OnToolsListed,
                                          weak_ptr_factory_.GetWeakPtr(),
                                          std::string(server_id)));
}

mojom::MCPServerStatus MCPHost::GetServerStatus(
    std::string_view server_id) const {
  if (const Client* client = FindClient(server_id)) {
    return client->health.status();
  }
  if (auto it = unusable_statuses_.find(server_id);
      it != unusable_statuses_.end()) {
    return it->second;
  }
  return mojom::MCPServerStatus::kUnavailable;
}

bool MCPHost::IsFailing(std::string_view server_id) const {
  const Client* client = FindClient(server_id);
  return client && client->health.IsFailing();
}

void MCPHost::MarkNotResponding(std::string_view server_id) {
  Client* client = FindClient(server_id);
  if (!client) {
    return;
  }
  const Statuses old_statuses = GetStatuses();
  client->health.OnListingLate();
  NotifyStatusChanges(old_statuses);
}

base::ScopedClosureRunner MCPHost::UseServer(std::string_view server_id) {
  std::string id(server_id);
  if (++server_uses_[id] == 1) {
    if (Client* client = FindClient(id)) {
      client->health.SetInUse(true);
    }
  }
  return base::ScopedClosureRunner(base::BindOnce(
      &MCPHost::ReleaseServer, weak_ptr_factory_.GetWeakPtr(), std::move(id)));
}

void MCPHost::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MCPHost::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
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
  // Dropped if the client is replaced or destroyed first.
  GetTools(id,
           base::BindOnce(&OnServerTools,
                          mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                              std::move(callback),
                              ServerToolsResult(base::unexpected(
                                  mojom::MCPServerError::kServerChanged)))));
}

void MCPHost::GetServerStatuses(GetServerStatusesCallback callback) {
  std::move(callback).Run(GetStatuses());
}

void MCPHost::ReconnectServer(const std::string& id) {
  Client* client = FindClient(id);
  if (!client || !client->health.IsFailing()) {
    return;
  }
  const Statuses old_statuses = GetStatuses();
  if (client->health.is_listing()) {
    // Its answer is the one wanted.
    client->health.StartChecking();
  } else if (client->health.CanReconnect()) {
    client->health.StartChecking();
    ListToolsAfresh(id);
  }
  NotifyStatusChanges(old_statuses);
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
  clients_ready_.Signal();
}

void MCPHost::BindReady(mojo::PendingReceiver<mojom::MCPHost> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void MCPHost::UpdateClients(const std::vector<mojom::MCPServerPtr>& servers) {
  const Statuses old_statuses = GetStatuses();
  std::map<std::string, Client, std::less<>> clients;
  std::map<std::string, mojom::MCPServerStatus, std::less<>> unusable_statuses;
  bool changed = false;
  for (const mojom::MCPServerPtr& server : servers) {
    if (!server->enabled || server->token_unreadable) {
      unusable_statuses.emplace(
          server->id, server->enabled ? mojom::MCPServerStatus::kAuthRequired
                                      : mojom::MCPServerStatus::kUnavailable);
      continue;
    }
    std::optional<MCPServerConfig> config =
        registry_.GetClientConfig(server->id);
    if (!config) {
      unusable_statuses.emplace(server->id,
                                mojom::MCPServerStatus::kUnavailable);
      continue;
    }
    if (auto it = clients_.find(server->id);
        it != clients_.end() && it->second.config == *config) {
      clients.insert(clients_.extract(it));
      continue;
    }
    base::expected<std::unique_ptr<MCPClient>, MCPError> client =
        MCPClient::Create(*config, url_loader_factory_);
    if (!client.has_value()) {
      unusable_statuses.emplace(
          server->id, GetMCPServerStatusForError(client.error().code));
      continue;
    }
    auto [it, inserted] = clients.try_emplace(
        server->id, std::move(*config), std::move(*client),
        base::BindRepeating(&MCPHost::ListToolsAfresh, base::Unretained(this),
                            server->id));
    it->second.health.SetInUse(server_uses_.contains(server->id));
    changed = true;
  }
  // The clients of servers that were removed, disabled or changed are
  // destroyed after the swap, so that anything they run as they go sees the
  // current ones.
  std::swap(clients_, clients);
  unusable_statuses_ = std::move(unusable_statuses);
  changed |= !clients.empty();
  clients.clear();
  if (changed) {
    observers_.Notify(&Observer::OnMCPClientsChanged);
  }
  NotifyStatusChanges(old_statuses);
}

MCPHost::Client* MCPHost::FindClient(std::string_view server_id) {
  auto it = clients_.find(server_id);
  return it == clients_.end() ? nullptr : &it->second;
}

const MCPHost::Client* MCPHost::FindClient(std::string_view server_id) const {
  auto it = clients_.find(server_id);
  return it == clients_.end() ? nullptr : &it->second;
}

void MCPHost::OnToolsListed(const std::string& server_id,
                            MCPClient::ToolsResult result) {
  Client* client = FindClient(server_id);
  if (!client) {
    return;
  }
  const Statuses old_statuses = GetStatuses();
  if (result.has_value()) {
    client->health.OnListingSucceeded();
  } else {
    client->health.OnListingFailed(result.error().code);
  }
  std::vector<MCPClient::GetToolsCallback> callbacks =
      std::exchange(client->listing_callbacks, {});
  base::WeakPtr<MCPHost> self = weak_ptr_factory_.GetWeakPtr();
  NotifyStatusChanges(old_statuses);
  for (size_t i = 0; i < callbacks.size() && self; ++i) {
    std::move(callbacks[i])
        .Run(i + 1 < callbacks.size() ? CloneResult(result)
                                      : std::move(result));
  }
}

void MCPHost::ListToolsAfresh(const std::string& server_id) {
  Client* client = FindClient(server_id);
  if (!client) {
    return;
  }
  client->client->EndSession();
  GetTools(server_id, base::DoNothing());
}

void MCPHost::ReleaseServer(const std::string& server_id) {
  auto it = server_uses_.find(server_id);
  CHECK(it != server_uses_.end());
  if (--it->second > 0) {
    return;
  }
  server_uses_.erase(it);
  if (Client* client = FindClient(server_id)) {
    client->health.SetInUse(false);
  }
}

MCPHost::Statuses MCPHost::GetStatuses() const {
  Statuses statuses(unusable_statuses_.begin(), unusable_statuses_.end());
  for (const auto& [id, client] : clients_) {
    statuses.emplace(id, client.health.status());
  }
  return statuses;
}

void MCPHost::NotifyStatusChanges(const Statuses& old_statuses) {
  const Statuses statuses = GetStatuses();
  if (statuses == old_statuses) {
    return;
  }
  base::flat_set<std::string> changed_ids;
  for (const auto& [id, status] : statuses) {
    if (auto it = old_statuses.find(id);
        it == old_statuses.end() || it->second != status) {
      changed_ids.insert(id);
    }
  }
  for (const auto& [id, status] : old_statuses) {
    if (!statuses.contains(id)) {
      changed_ids.insert(id);
    }
  }
  for (const std::string& id : changed_ids) {
    observers_.Notify(&Observer::OnMCPServerStatusChanged, id);
  }
  for (auto& observer : remote_observers_) {
    observer->OnServerStatusesChanged(statuses);
  }
}

}  // namespace ai_chat
