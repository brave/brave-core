/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HOST_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HOST_H_

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "base/scoped_observation.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_server_health.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_server_registry.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class OSCryptAsync;
}  // namespace os_crypt_async

namespace ai_chat {

// The profile's MCP host: owns the user's server configs, and a client for
// each enabled server, which is replaced or destroyed as its config changes.
// Destroying it ends the clients' sessions, so it must go before the network
// does.
//
// It also keeps each enabled server's status, which every listing of its
// tools updates. Once one fails, callers shouldn't wait for the server, and
// it's listed again in the background, while it's in use, until that works.
class MCPHost : public mojom::MCPHost, public MCPServerRegistry::Observer {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Clients were created, replaced or destroyed to match the servers'
    // configs.
    virtual void OnMCPClientsChanged() {}
    // GetServerStatus(server_id) changed.
    virtual void OnMCPServerStatusChanged(const std::string& server_id) {}
  };

  struct EnabledServer {
    EnabledServer();
    EnabledServer(EnabledServer&&) noexcept;
    EnabledServer& operator=(EnabledServer&&) noexcept;
    ~EnabledServer();

    mojom::MCPServerPtr server;
    base::WeakPtr<MCPClient> client;
  };

  MCPHost(PrefService& prefs,
          os_crypt_async::OSCryptAsync& os_crypt_async,
          scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  MCPHost(const MCPHost&) = delete;
  MCPHost& operator=(const MCPHost&) = delete;
  ~MCPHost() override;

  // Messages are handled once tokens can be read and written.
  void Bind(mojo::PendingReceiver<mojom::MCPHost> receiver);

  // Enabled servers that have a client, in the order they were added. Empty
  // until tokens can be decrypted. A server whose token can't be decrypted
  // has no client.
  std::vector<EnabledServer> GetEnabledServers() const;
  base::WeakPtr<MCPClient> GetClient(std::string_view server_id) const;

  // Whether the clients have been created, which waits for tokens to be
  // decryptable.
  bool IsReady() const;
  // Runs `callback` once IsReady(). Always posted.
  void RunWhenReady(base::OnceClosure callback);

  // Lists the tools of `server_id`, as its client's GetTools() does, joining
  // a listing that's under way, and updates its status with the result.
  // `callback` is dropped if the server has no client, or its client is
  // replaced or destroyed first.
  void GetTools(std::string_view server_id,
                MCPClient::GetToolsCallback callback);
  mojom::MCPServerStatus GetServerStatus(std::string_view server_id) const;
  // Whether listing the tools of `server_id` has failed, and hasn't worked
  // since, so callers shouldn't wait for them.
  bool IsFailing(std::string_view server_id) const;
  // A caller stopped waiting for the tools of `server_id`.
  void MarkNotResponding(std::string_view server_id);
  // While the returned object is held, a server whose listing failed is
  // listed again in the background.
  [[nodiscard]] base::ScopedClosureRunner UseServer(std::string_view server_id);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // mojom::MCPHost:
  void GetServers(GetServersCallback callback) override;
  void AddServer(mojom::MCPServerParamsPtr params,
                 const std::optional<std::string>& token,
                 AddServerCallback callback) override;
  void UpdateServer(const std::string& id,
                    mojom::MCPServerParamsPtr params,
                    const std::optional<std::string>& token,
                    UpdateServerCallback callback) override;
  void ClearServerToken(const std::string& id,
                        ClearServerTokenCallback callback) override;
  void RemoveServer(const std::string& id,
                    RemoveServerCallback callback) override;
  void GetServerTools(const std::string& id,
                      GetServerToolsCallback callback) override;
  void GetServerStatuses(GetServerStatusesCallback callback) override;
  void ReconnectServer(const std::string& id) override;
  void BindObserver(
      mojo::PendingRemote<mojom::MCPHostObserver> observer) override;

 private:
  using Statuses = base::flat_map<std::string, mojom::MCPServerStatus>;

  struct Client {
    Client(MCPServerConfig config,
           std::unique_ptr<MCPClient> client,
           base::RepeatingClosure retry);
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    ~Client();

    // What `client` was created from.
    MCPServerConfig config;
    std::unique_ptr<MCPClient> client;
    MCPServerHealth health;
    // The callers of the listing under way.
    std::vector<MCPClient::GetToolsCallback> listing_callbacks;
  };

  // MCPServerRegistry::Observer:
  void OnMCPServersChanged() override;

  void OnRegistryReady();
  void BindReady(mojo::PendingReceiver<mojom::MCPHost> receiver);
  // Creates, replaces and destroys clients to match `servers`, and tells
  // observers if it did.
  void UpdateClients(const std::vector<mojom::MCPServerPtr>& servers);

  Client* FindClient(std::string_view server_id);
  const Client* FindClient(std::string_view server_id) const;
  void OnToolsListed(const std::string& server_id,
                     MCPClient::ToolsResult result);
  // Lists the tools of `server_id` in a new session, so that a server that's
  // lost the last one starts another.
  void ListToolsAfresh(const std::string& server_id);
  void ReleaseServer(const std::string& server_id);
  Statuses GetStatuses() const;
  // Tells observers of each status that's changed since `old_statuses`.
  void NotifyStatusChanges(const Statuses& old_statuses);

  // Outlives the clients, which use it as they're destroyed.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  MCPServerRegistry registry_;
  base::ScopedObservation<MCPServerRegistry, MCPServerRegistry::Observer>
      registry_observation_{this};
  // By server id.
  std::map<std::string, Client, std::less<>> clients_;
  // The status of each server that has no client, by id: why it has none.
  std::map<std::string, mojom::MCPServerStatus, std::less<>> unusable_statuses_;
  // How many users each server has, by id.
  std::map<std::string, size_t, std::less<>> server_uses_;
  // Signaled once the first clients have been created.
  base::OneShotEvent clients_ready_;
  base::ObserverList<Observer> observers_;
  mojo::ReceiverSet<mojom::MCPHost> receivers_;
  mojo::RemoteSet<mojom::MCPHostObserver> remote_observers_;
  base::WeakPtrFactory<MCPHost> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HOST_H_
