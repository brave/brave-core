/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_server_registry.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/types/expected_macros.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_endpoint_validator.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/http/http_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace ai_chat {

namespace {

// Servers saved in any other version are ignored, and left as they are.
constexpr int kPrefVersion = 1;

// The pref is {"version": kPrefVersion, "servers": [<server>, ...]}.
constexpr char kVersionKey[] = "version";
constexpr char kServersKey[] = "servers";

// Server keys.
constexpr char kIdKey[] = "id";
constexpr char kNameKey[] = "name";
constexpr char kSlugKey[] = "slug";
constexpr char kEndpointKey[] = "endpoint";
constexpr char kTransportKey[] = "transport";
constexpr char kEnabledKey[] = "enabled";
constexpr char kAutoAttachKey[] = "auto_attach";
// OSCrypt ciphertext, base64-encoded.
constexpr char kTokenKey[] = "token";

constexpr char kStreamableHttpTransport[] = "streamable_http";

// For a name with no ASCII letters or digits.
constexpr char kDefaultSlug[] = "server";

bool IsValidSlug(std::string_view slug) {
  return !slug.empty() && slug.size() <= kMaxMCPServerSlugLength &&
         std::ranges::all_of(slug, [](char c) {
           return base::IsAsciiLower(c) || base::IsAsciiDigit(c);
         });
}

// Null if `dict` isn't a well-formed server.
mojom::MCPServerPtr ServerFromDict(const base::DictValue& dict) {
  const std::string* id = dict.FindString(kIdKey);
  const std::string* name = dict.FindString(kNameKey);
  const std::string* slug = dict.FindString(kSlugKey);
  const std::string* endpoint = dict.FindString(kEndpointKey);
  const std::string* transport = dict.FindString(kTransportKey);
  const std::optional<bool> enabled = dict.FindBool(kEnabledKey);
  const std::optional<bool> auto_attach = dict.FindBool(kAutoAttachKey);
  if (!id || id->empty() || !name || !slug || !IsValidSlug(*slug) ||
      !endpoint || !transport || *transport != kStreamableHttpTransport ||
      !enabled || !auto_attach) {
    return nullptr;
  }
  GURL endpoint_url(*endpoint);
  if (!endpoint_url.is_valid()) {
    return nullptr;
  }
  return mojom::MCPServer::New(*id, *name, std::move(endpoint_url), *enabled,
                               *auto_attach, *slug,
                               dict.FindString(kTokenKey) != nullptr,
                               /*token_unreadable=*/false);
}

const base::ListValue* FindServerList(const base::DictValue& pref) {
  if (pref.FindInt(kVersionKey) != kPrefVersion) {
    return nullptr;
  }
  return pref.FindList(kServersKey);
}

struct ParsedServer {
  // In the pref's server list.
  size_t index;
  mojom::MCPServerPtr server;
};

// Skips malformed servers, and any with the id or slug of an earlier one, so
// that both stay unique.
std::vector<ParsedServer> ParseServers(const base::DictValue& pref) {
  std::vector<ParsedServer> servers;
  const base::ListValue* list = FindServerList(pref);
  if (!list) {
    return servers;
  }
  base::flat_set<std::string> ids;
  base::flat_set<std::string> slugs;
  for (size_t i = 0; i < list->size(); ++i) {
    const base::DictValue* dict = (*list)[i].GetIfDict();
    mojom::MCPServerPtr server = dict ? ServerFromDict(*dict) : nullptr;
    if (!server || ids.contains(server->id) || slugs.contains(server->slug)) {
      continue;
    }
    ids.insert(server->id);
    slugs.insert(server->slug);
    servers.push_back({i, std::move(server)});
  }
  return servers;
}

const base::DictValue& ServerDict(const base::DictValue& pref, size_t index) {
  return (*FindServerList(pref))[index].GetDict();
}

std::optional<size_t> FindServerIndex(const base::DictValue& pref,
                                      std::string_view id) {
  for (const ParsedServer& parsed : ParseServers(pref)) {
    if (parsed.server->id == id) {
      return parsed.index;
    }
  }
  return std::nullopt;
}

// Whether the servers in `pref` can be changed: they're saved in this version,
// or there are none yet. Another version's are kept intact, so that they're
// still there for the version that saved them.
bool IsWritable(const base::DictValue& pref) {
  const base::Value* version = pref.Find(kVersionKey);
  return !version || version->GetIfInt() == kPrefVersion;
}

base::ListValue& GetMutableServerList(ScopedDictPrefUpdate& update) {
  base::DictValue& pref = update.Get();
  CHECK(IsWritable(pref));
  if (!pref.contains(kVersionKey)) {
    pref.Set(kVersionKey, kPrefVersion);
  }
  return *pref.EnsureList(kServersKey);
}

// Returns the name to store.
base::expected<std::string, mojom::MCPServerError> ValidateName(
    std::string_view name) {
  const std::string_view trimmed =
      base::TrimWhitespaceASCII(name, base::TRIM_ALL);
  if (trimmed.empty() || trimmed.size() > kMaxMCPServerNameLength ||
      !base::IsStringUTF8(trimmed) || std::ranges::any_of(trimmed, [](char c) {
        return base::IsAsciiControl(c);
      })) {
    return base::unexpected(mojom::MCPServerError::kInvalidName);
  }
  return std::string(trimmed);
}

std::optional<mojom::MCPServerError> ValidateEndpoint(const GURL& endpoint) {
  if (endpoint.spec().size() > kMaxMCPServerEndpointLength) {
    return mojom::MCPServerError::kEndpointTooLong;
  }
  switch (ValidateMCPEndpoint(endpoint)) {
    case MCPEndpointValidationResult::kValid:
      return std::nullopt;
    case MCPEndpointValidationResult::kInvalidUrl:
      return mojom::MCPServerError::kInvalidUrl;
    case MCPEndpointValidationResult::kUnsupportedScheme:
      return mojom::MCPServerError::kUnsupportedScheme;
    case MCPEndpointValidationResult::kEmbeddedCredentials:
      return mojom::MCPServerError::kEmbeddedCredentials;
    case MCPEndpointValidationResult::kInsecureScheme:
      return mojom::MCPServerError::kInsecureScheme;
    case MCPEndpointValidationResult::kPrivateAddressNotAllowed:
      return mojom::MCPServerError::kPrivateAddressNotAllowed;
  }
  NOTREACHED();
}

// Derived from the name, and unique among `taken`. Only lowercase ASCII
// letters and digits, so that it can't run into the tool name that follows it.
std::string MakeSlug(std::string_view name,
                     const base::flat_set<std::string>& taken) {
  std::string base;
  for (char c : name) {
    if (base.size() == kMaxMCPServerSlugLength) {
      break;
    }
    if (base::IsAsciiAlphaNumeric(c)) {
      base.push_back(base::ToLowerASCII(c));
    }
  }
  if (base.empty()) {
    base = kDefaultSlug;
  }
  std::string slug = base;
  for (size_t n = 2; taken.contains(slug); ++n) {
    const std::string suffix = base::NumberToString(n);
    slug = base::StrCat(
        {std::string_view(base).substr(
             0, std::min(base.size(), kMaxMCPServerSlugLength - suffix.size())),
         suffix});
  }
  return slug;
}

void WriteParams(const mojom::MCPServerParams& params,
                 std::string name,
                 base::DictValue& dict) {
  dict.Set(kNameKey, std::move(name));
  dict.Set(kEndpointKey, params.endpoint.spec());
  dict.Set(kEnabledKey, params.enabled);
  dict.Set(kAutoAttachKey, params.auto_attach);
}

}  // namespace

MCPServerRegistry::MCPServerRegistry(
    PrefService& prefs,
    os_crypt_async::OSCryptAsync& os_crypt_async)
    : prefs_(prefs) {
  pref_change_registrar_.Init(&prefs);
  pref_change_registrar_.Add(
      prefs::kBraveAIChatMCPServers,
      base::BindRepeating(&MCPServerRegistry::OnPrefChanged,
                          base::Unretained(this)));
  os_crypt_async.GetInstance(base::BindOnce(
      &MCPServerRegistry::OnEncryptorReady, weak_ptr_factory_.GetWeakPtr()));
}

MCPServerRegistry::~MCPServerRegistry() = default;

std::vector<mojom::MCPServerPtr> MCPServerRegistry::GetServers() const {
  const base::DictValue& pref = prefs_->GetDict(prefs::kBraveAIChatMCPServers);
  std::vector<mojom::MCPServerPtr> servers;
  for (ParsedServer& parsed : ParseServers(pref)) {
    SetTokenStatus(ServerDict(pref, parsed.index), *parsed.server);
    servers.push_back(std::move(parsed.server));
  }
  return servers;
}

mojom::MCPServerPtr MCPServerRegistry::GetServer(std::string_view id) const {
  const base::DictValue& pref = prefs_->GetDict(prefs::kBraveAIChatMCPServers);
  for (ParsedServer& parsed : ParseServers(pref)) {
    if (parsed.server->id == id) {
      SetTokenStatus(ServerDict(pref, parsed.index), *parsed.server);
      return std::move(parsed.server);
    }
  }
  return nullptr;
}

MCPServerRegistry::Result MCPServerRegistry::AddServer(
    const mojom::MCPServerParams& params,
    std::optional<std::string> token) {
  if (!IsWritable(prefs_->GetDict(prefs::kBraveAIChatMCPServers))) {
    return base::unexpected(mojom::MCPServerError::kUnsupportedSettingsVersion);
  }
  ASSIGN_OR_RETURN(std::string name, ValidateName(params.name));
  if (std::optional<mojom::MCPServerError> error =
          ValidateEndpoint(params.endpoint)) {
    return base::unexpected(*error);
  }
  const std::vector<mojom::MCPServerPtr> servers = GetServers();
  if (servers.size() >= kMaxMCPServers) {
    return base::unexpected(mojom::MCPServerError::kTooManyServers);
  }
  std::optional<std::string> encrypted_token;
  if (token) {
    ASSIGN_OR_RETURN(encrypted_token, PrepareToken(*token));
  }

  base::flat_set<std::string> slugs;
  for (const mojom::MCPServerPtr& server : servers) {
    slugs.insert(server->slug);
  }
  base::DictValue dict;
  dict.Set(kIdKey, base::Uuid::GenerateRandomV4().AsLowercaseString());
  dict.Set(kSlugKey, MakeSlug(name, slugs));
  dict.Set(kTransportKey, kStreamableHttpTransport);
  WriteParams(params, std::move(name), dict);
  if (encrypted_token) {
    dict.Set(kTokenKey, std::move(*encrypted_token));
  }
  mojom::MCPServerPtr server = ServerFromDict(dict);
  CHECK(server);

  ScopedDictPrefUpdate update(&prefs_.get(), prefs::kBraveAIChatMCPServers);
  GetMutableServerList(update).Append(std::move(dict));
  return server;
}

MCPServerRegistry::Result MCPServerRegistry::UpdateServer(
    std::string_view id,
    const mojom::MCPServerParams& params,
    std::optional<std::string> token) {
  const base::DictValue& pref = prefs_->GetDict(prefs::kBraveAIChatMCPServers);
  if (!IsWritable(pref)) {
    return base::unexpected(mojom::MCPServerError::kUnsupportedSettingsVersion);
  }
  const std::optional<size_t> index = FindServerIndex(pref, id);
  if (!index) {
    return base::unexpected(mojom::MCPServerError::kNotFound);
  }
  ASSIGN_OR_RETURN(std::string name, ValidateName(params.name));
  if (std::optional<mojom::MCPServerError> error =
          ValidateEndpoint(params.endpoint)) {
    return base::unexpected(*error);
  }
  std::optional<std::string> encrypted_token;
  if (token) {
    ASSIGN_OR_RETURN(encrypted_token, PrepareToken(*token));
  }

  ScopedDictPrefUpdate update(&prefs_.get(), prefs::kBraveAIChatMCPServers);
  base::DictValue& dict = GetMutableServerList(update)[*index].GetDict();
  // The token was given for the old origin, and mustn't be sent anywhere else.
  const bool origin_changed =
      !url::Origin::Create(GURL(*dict.FindString(kEndpointKey)))
           .IsSameOriginWith(url::Origin::Create(params.endpoint));
  WriteParams(params, std::move(name), dict);
  if (encrypted_token) {
    dict.Set(kTokenKey, std::move(*encrypted_token));
  } else if (origin_changed) {
    dict.Remove(kTokenKey);
  }
  mojom::MCPServerPtr server = ServerFromDict(dict);
  CHECK(server);
  SetTokenStatus(dict, *server);
  return server;
}

MCPServerRegistry::Result MCPServerRegistry::ClearServerToken(
    std::string_view id) {
  const base::DictValue& pref = prefs_->GetDict(prefs::kBraveAIChatMCPServers);
  if (!IsWritable(pref)) {
    return base::unexpected(mojom::MCPServerError::kUnsupportedSettingsVersion);
  }
  const std::optional<size_t> index = FindServerIndex(pref, id);
  if (!index) {
    return base::unexpected(mojom::MCPServerError::kNotFound);
  }
  ScopedDictPrefUpdate update(&prefs_.get(), prefs::kBraveAIChatMCPServers);
  base::DictValue& dict = GetMutableServerList(update)[*index].GetDict();
  dict.Remove(kTokenKey);
  mojom::MCPServerPtr server = ServerFromDict(dict);
  CHECK(server);
  return server;
}

std::optional<mojom::MCPServerError> MCPServerRegistry::RemoveServer(
    std::string_view id) {
  const base::DictValue& pref = prefs_->GetDict(prefs::kBraveAIChatMCPServers);
  if (!IsWritable(pref)) {
    return mojom::MCPServerError::kUnsupportedSettingsVersion;
  }
  const std::optional<size_t> index = FindServerIndex(pref, id);
  if (!index) {
    return mojom::MCPServerError::kNotFound;
  }
  ScopedDictPrefUpdate update(&prefs_.get(), prefs::kBraveAIChatMCPServers);
  base::ListValue& list = GetMutableServerList(update);
  list.erase(list.begin() + *index);
  return std::nullopt;
}

std::optional<MCPServerConfig> MCPServerRegistry::GetClientConfig(
    std::string_view id) const {
  const base::DictValue& pref = prefs_->GetDict(prefs::kBraveAIChatMCPServers);
  const std::optional<size_t> index = FindServerIndex(pref, id);
  if (!index) {
    return std::nullopt;
  }
  const base::DictValue& dict = ServerDict(pref, *index);
  MCPServerConfig config(GURL(*dict.FindString(kEndpointKey)),
                         /*bearer_token=*/std::nullopt,
                         *dict.FindString(kNameKey));
  if (const std::string* encrypted_token = dict.FindString(kTokenKey)) {
    config.bearer_token = DecryptToken(*encrypted_token);
    // Connecting without the token would only fail, or reach a server that
    // has less access than the user gave it.
    if (!config.bearer_token) {
      VLOG(1) << "Couldn't decrypt the access token of an MCP server";
      return std::nullopt;
    }
  }
  return config;
}

void MCPServerRegistry::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MCPServerRegistry::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MCPServerRegistry::OnEncryptorReady(
    scoped_refptr<os_crypt_async::Encryptor> encryptor) {
  encryptor_ = std::move(encryptor);
  ready_.Signal();
}

void MCPServerRegistry::OnPrefChanged() {
  observers_.Notify(&Observer::OnMCPServersChanged);
}

std::optional<std::string> MCPServerRegistry::EncryptToken(
    const std::string& token) const {
  std::string ciphertext;
  if (!encryptor_ || !encryptor_->EncryptString(token, &ciphertext)) {
    return std::nullopt;
  }
  return base::Base64Encode(ciphertext);
}

std::optional<std::string> MCPServerRegistry::DecryptToken(
    std::string_view encoded) const {
  std::string ciphertext;
  std::string token;
  if (!encryptor_ || !base::Base64Decode(encoded, &ciphertext) ||
      !encryptor_->DecryptString(ciphertext, &token)) {
    return std::nullopt;
  }
  return token;
}

void MCPServerRegistry::SetTokenStatus(const base::DictValue& dict,
                                       mojom::MCPServer& server) const {
  // Unknown until there's an encryptor to try.
  server.token_unreadable = server.has_token && encryptor_ &&
                            !DecryptToken(*dict.FindString(kTokenKey));
}

base::expected<std::string, mojom::MCPServerError>
MCPServerRegistry::PrepareToken(std::string_view token) const {
  const std::string_view trimmed =
      base::TrimWhitespaceASCII(token, base::TRIM_ALL);
  if (trimmed.empty() || trimmed.size() > kMaxMCPServerTokenLength ||
      !net::HttpUtil::IsValidHeaderValue(trimmed)) {
    return base::unexpected(mojom::MCPServerError::kInvalidToken);
  }
  std::optional<std::string> encrypted = EncryptToken(std::string(trimmed));
  if (!encrypted) {
    return base::unexpected(mojom::MCPServerError::kTokenEncryptionFailed);
  }
  return std::move(*encrypted);
}

}  // namespace ai_chat
