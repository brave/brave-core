/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HOST_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HOST_H_

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "base/scoped_observation.h"
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

class MCPClient;

// The profile's MCP host: owns the user's server configs, and a client for
// each enabled server, which is replaced or destroyed as its config changes.
// Destroying it ends the clients' sessions, so it must go before the network
// does.
class MCPHost : public mojom::MCPHost, public MCPServerRegistry::Observer {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Clients were created, replaced or destroyed to match the servers'
    // configs.
    virtual void OnMCPClientsChanged() {}
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
  void BindObserver(
      mojo::PendingRemote<mojom::MCPHostObserver> observer) override;

 private:
  struct Client {
    Client(MCPServerConfig config, std::unique_ptr<MCPClient> client);
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;
    ~Client();

    // What `client` was created from.
    MCPServerConfig config;
    std::unique_ptr<MCPClient> client;
  };

  // MCPServerRegistry::Observer:
  void OnMCPServersChanged() override;

  void OnRegistryReady();
  void BindReady(mojo::PendingReceiver<mojom::MCPHost> receiver);
  // Creates, replaces and destroys clients to match `servers`, and tells
  // observers if it did.
  void UpdateClients(const std::vector<mojom::MCPServerPtr>& servers);

  // Outlives the clients, which use it as they're destroyed.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  MCPServerRegistry registry_;
  base::ScopedObservation<MCPServerRegistry, MCPServerRegistry::Observer>
      registry_observation_{this};
  // By server id.
  std::map<std::string, Client, std::less<>> clients_;
  // Signaled once the first clients have been created.
  base::OneShotEvent clients_ready_;
  base::ObserverList<Observer> observers_;
  mojo::ReceiverSet<mojom::MCPHost> receivers_;
  mojo::RemoteSet<mojom::MCPHostObserver> remote_observers_;
  base::WeakPtrFactory<MCPHost> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_HOST_H_
