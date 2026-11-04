/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_SERVER_REGISTRY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_SERVER_REGISTRY_H_

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

namespace ai_chat {

inline constexpr size_t kMaxMCPServers = 32;
// In bytes.
inline constexpr size_t kMaxMCPServerNameLength = 100;
inline constexpr size_t kMaxMCPServerEndpointLength = 2 * 1024;
inline constexpr size_t kMaxMCPServerSlugLength = 16;
inline constexpr size_t kMaxMCPServerTokenLength = 8 * 1024;

// The user's remote MCP servers, kept in prefs. Access tokens are stored
// encrypted, and only decrypted to create a client.
class MCPServerRegistry {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // A server was added, changed or removed.
    virtual void OnMCPServersChanged() {}
  };

  using Result = base::expected<mojom::MCPServerPtr, mojom::MCPServerError>;

  MCPServerRegistry(PrefService& prefs,
                    os_crypt_async::OSCryptAsync& os_crypt_async);
  MCPServerRegistry(const MCPServerRegistry&) = delete;
  MCPServerRegistry& operator=(const MCPServerRegistry&) = delete;
  ~MCPServerRegistry();

  // Signaled once tokens can be encrypted and decrypted. Until then, setting a
  // token fails with kTokenEncryptionFailed, and there's no client config for
  // a server that has one.
  const base::OneShotEvent& ready() const { return ready_; }

  // In the order they were added.
  std::vector<mojom::MCPServerPtr> GetServers() const;
  // Null if there's no server `id`.
  mojom::MCPServerPtr GetServer(std::string_view id) const;

  // `token`, if set, is sent to the server as a bearer token.
  Result AddServer(const mojom::MCPServerParams& params,
                   std::optional<std::string> token);
  // Replaces the server's token if `token` is set. Otherwise keeps it, unless
  // the endpoint moves to another origin, which the token isn't for.
  Result UpdateServer(std::string_view id,
                      const mojom::MCPServerParams& params,
                      std::optional<std::string> token);
  Result ClearServerToken(std::string_view id);
  // Returns nullopt once the server is removed.
  std::optional<mojom::MCPServerError> RemoveServer(std::string_view id);

  // What a client for server `id` is created from, including its decrypted
  // token. nullopt if there's no server `id`, or its token can't be decrypted.
  std::optional<MCPServerConfig> GetClientConfig(std::string_view id) const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void OnEncryptorReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);
  void OnPrefChanged();

  // Returns the base64-encoded ciphertext, or nullopt if it can't be
  // encrypted.
  std::optional<std::string> EncryptToken(const std::string& token) const;
  std::optional<std::string> DecryptToken(std::string_view encoded) const;

  // Sets `server.token_unreadable` from the server's pref, `dict`.
  void SetTokenStatus(const base::DictValue& dict,
                      mojom::MCPServer& server) const;

  // Validates `token` and returns it encrypted.
  base::expected<std::string, mojom::MCPServerError> PrepareToken(
      std::string_view token) const;

  PrefChangeRegistrar pref_change_registrar_;
  // Null until `ready_` is signaled.
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  base::OneShotEvent ready_;
  base::ObserverList<Observer> observers_;
  const raw_ref<PrefService> prefs_;
  base::WeakPtrFactory<MCPServerRegistry> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MCP_MCP_SERVER_REGISTRY_H_
