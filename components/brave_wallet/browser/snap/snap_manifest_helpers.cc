/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_manifest_helpers.h"

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

namespace brave_wallet {

namespace {

constexpr const char* kAllowedPermissions[] = {
    "snap_getBip44Entropy",
    "snap_getBip32Entropy",
    "snap_getEntropy",
    "snap_dialog",
    "snap_confirm",
    "snap_notify",
    "snap_manageState",
    "endowment:network-access",
    "endowment:rpc",
    "endowment:webassembly",
    "endowment:page-home",
    "endowment:lifecycle-hooks",
    "endowment:cronjob",
    "endowment:transaction-insight",
    "endowment:signature-insight",
    "endowment:ethereum-provider",
    "endowment:name-lookup",
};

constexpr char kMaxRequestTimeKey[] = "max_request_time";

std::optional<int32_t> ReadMaxRequestTime(const base::DictValue& dict) {
  if (const std::optional<double> max_time = dict.FindDouble("maxRequestTime")) {
    return static_cast<int32_t>(*max_time);
  }
  return std::nullopt;
}

void WriteMaxRequestTime(const std::optional<int32_t>& max_request_time,
                         base::DictValue& dict) {
  if (max_request_time.has_value()) {
    dict.Set(kMaxRequestTimeKey, *max_request_time);
  }
}

void ReadMaxRequestTimeFromStored(const base::DictValue& dict,
                                  std::optional<int32_t>& out) {
  if (const std::optional<double> max_time =
          dict.FindDouble(kMaxRequestTimeKey)) {
    out = static_cast<int32_t>(*max_time);
  }
}

void ReadStringList(const base::DictValue& dict,
                    std::string_view list_name,
                    std::vector<std::string>& out) {
  const base::ListValue* list = dict.FindList(list_name);
  if (!list) {
    return;
  }
  for (const auto& item : *list) {
    if (item.is_string()) {
      out.push_back(item.GetString());
    }
  }
}

base::ListValue StringVectorToList(const std::vector<std::string>& items) {
  base::ListValue list;
  for (const auto& item : items) {
    list.Append(item);
  }
  return list;
}

mojom::SnapSimpleConfigPtr BuildSimpleConfig(const base::Value& value) {
  auto config = mojom::SnapSimpleConfig::New();
  if (value.is_dict()) {
    config->max_request_time = ReadMaxRequestTime(value.GetDict());
  }
  return config;
}

mojom::SnapRpcConfigPtr BuildRpcConfig(const base::Value& value) {
  auto config = mojom::SnapRpcConfig::New();
  if (!value.is_dict()) {
    return config;
  }
  const base::DictValue& rpc = value.GetDict();
  config->allow_dapps = rpc.FindBool("dapps").value_or(false);
  config->allow_snaps = rpc.FindBool("snaps").value_or(false);
  ReadStringList(rpc, "allowedOrigins", config->allowed_origins);
  config->max_request_time = ReadMaxRequestTime(rpc);
  return config;
}

mojom::SnapNameLookupConfigPtr BuildNameLookupConfig(const base::Value& value) {
  auto config = mojom::SnapNameLookupConfig::New();
  if (!value.is_dict()) {
    return config;
  }
  const base::DictValue& cfg = value.GetDict();
  ReadStringList(cfg, "chains", config->chains);
  if (const base::DictValue* matchers = cfg.FindDict("matchers")) {
    auto matcher_config = mojom::SnapNameLookupMatchers::New();
    ReadStringList(*matchers, "tlds", matcher_config->tlds);
    ReadStringList(*matchers, "schemes", matcher_config->schemes);
    config->matchers = std::move(matcher_config);
  }
  config->max_request_time = ReadMaxRequestTime(cfg);
  return config;
}

mojom::SnapCronjobRequestPtr BuildCronjobRequest(const base::DictValue& dict) {
  auto request = mojom::SnapCronjobRequest::New();
  if (const std::string* method = dict.FindString("method")) {
    request->method = *method;
  }
  if (const base::Value* params = dict.Find("params")) {
    request->params = params->Clone();
  }
  return request;
}

mojom::SnapCronjobConfigPtr BuildCronjobConfig(const base::Value& value) {
  auto config = mojom::SnapCronjobConfig::New();
  if (!value.is_dict()) {
    return config;
  }
  const base::DictValue& cfg = value.GetDict();
  if (const base::ListValue* jobs = cfg.FindList("jobs")) {
    for (const auto& job_value : *jobs) {
      if (!job_value.is_dict()) {
        continue;
      }
      const base::DictValue& job_dict = job_value.GetDict();
      auto job = mojom::SnapCronjob::New();
      if (const std::string* expression = job_dict.FindString("expression")) {
        job->expression = *expression;
      }
      if (const base::DictValue* request = job_dict.FindDict("request")) {
        job->request = BuildCronjobRequest(*request);
      }
      config->jobs.push_back(std::move(job));
    }
  }
  config->max_request_time = ReadMaxRequestTime(cfg);
  return config;
}

mojom::SnapBoolConfigPtr BuildBoolConfig(const base::Value& value,
                                           std::string_view bool_key) {
  auto config = mojom::SnapBoolConfig::New();
  if (value.is_dict()) {
    const base::DictValue& dict = value.GetDict();
    config->value = dict.FindBool(bool_key).value_or(false);
    config->max_request_time = ReadMaxRequestTime(dict);
  }
  return config;
}

mojom::SnapKeyringConfigPtr BuildKeyringConfig(const base::Value& value) {
  auto config = mojom::SnapKeyringConfig::New();
  if (!value.is_dict()) {
    return config;
  }
  const base::DictValue& cfg = value.GetDict();
  ReadStringList(cfg, "allowedOrigins", config->allowed_origins);
  config->max_request_time = ReadMaxRequestTime(cfg);
  return config;
}

mojom::SnapBip44EntropyConfigPtr BuildBip44EntropyConfig(
    const base::Value& value) {
  auto config = mojom::SnapBip44EntropyConfig::New();
  if (!value.is_list()) {
    return config;
  }
  for (const auto& item : value.GetList()) {
    if (!item.is_dict()) {
      continue;
    }
    const base::DictValue& entry = item.GetDict();
    if (const std::optional<double> coin_type = entry.FindDouble("coinType")) {
      auto entropy = mojom::SnapBip44Entropy::New();
      entropy->coin_type = static_cast<int32_t>(*coin_type);
      config->entries.push_back(std::move(entropy));
    }
  }
  return config;
}

mojom::SnapBip32EntropyConfigPtr BuildBip32EntropyConfig(
    const base::Value& value) {
  auto config = mojom::SnapBip32EntropyConfig::New();
  if (!value.is_list()) {
    return config;
  }
  for (const auto& item : value.GetList()) {
    if (!item.is_dict()) {
      continue;
    }
    const base::DictValue& entry = item.GetDict();
    auto entropy = mojom::SnapBip32Entropy::New();
    ReadStringList(entry, "path", entropy->path);
    if (const std::string* curve = entry.FindString("curve")) {
      entropy->curve = *curve;
    }
    config->entries.push_back(std::move(entropy));
  }
  return config;
}

void SerializeSimpleConfig(const mojom::SnapSimpleConfig& config,
                           base::DictValue& dict) {
  WriteMaxRequestTime(config.max_request_time, dict);
}

void SerializeRpcConfig(const mojom::SnapRpcConfig& config,
                        base::DictValue& dict) {
  dict.Set("allow_dapps", config.allow_dapps);
  dict.Set("allow_snaps", config.allow_snaps);
  dict.Set("allowed_origins", StringVectorToList(config.allowed_origins));
  WriteMaxRequestTime(config.max_request_time, dict);
}

void SerializeNameLookupConfig(const mojom::SnapNameLookupConfig& config,
                               base::DictValue& dict) {
  dict.Set("chains", StringVectorToList(config.chains));
  if (config.matchers) {
    base::DictValue matchers;
    matchers.Set("tlds", StringVectorToList(config.matchers->tlds));
    matchers.Set("schemes", StringVectorToList(config.matchers->schemes));
    dict.Set("matchers", std::move(matchers));
  }
  WriteMaxRequestTime(config.max_request_time, dict);
}

void SerializeCronjobRequest(const mojom::SnapCronjobRequest& request,
                             base::DictValue& dict) {
  dict.Set("method", request.method);
  if (request.params) {
    dict.Set("params", request.params->Clone());
  }
}

void SerializeCronjobConfig(const mojom::SnapCronjobConfig& config,
                            base::DictValue& dict) {
  base::ListValue jobs;
  for (const auto& job : config.jobs) {
    base::DictValue job_dict;
    job_dict.Set("expression", job->expression);
    if (job->request) {
      base::DictValue request_dict;
      SerializeCronjobRequest(*job->request, request_dict);
      job_dict.Set("request", std::move(request_dict));
    }
    jobs.Append(std::move(job_dict));
  }
  dict.Set("jobs", std::move(jobs));
  WriteMaxRequestTime(config.max_request_time, dict);
}

void SerializeBoolConfig(const mojom::SnapBoolConfig& config,
                         base::DictValue& dict) {
  dict.Set("value", config.value);
  WriteMaxRequestTime(config.max_request_time, dict);
}

void SerializeKeyringConfig(const mojom::SnapKeyringConfig& config,
                            base::DictValue& dict) {
  dict.Set("allowed_origins", StringVectorToList(config.allowed_origins));
  WriteMaxRequestTime(config.max_request_time, dict);
}

void SerializeBip44EntropyConfig(const mojom::SnapBip44EntropyConfig& config,
                                 base::DictValue& dict) {
  base::ListValue entries;
  for (const auto& entry : config.entries) {
    base::DictValue entry_dict;
    entry_dict.Set("coin_type", entry->coin_type);
    entries.Append(std::move(entry_dict));
  }
  dict.Set("entries", std::move(entries));
}

void SerializeBip32EntropyConfig(const mojom::SnapBip32EntropyConfig& config,
                                 base::DictValue& dict) {
  base::ListValue entries;
  for (const auto& entry : config.entries) {
    base::DictValue entry_dict;
    entry_dict.Set("path", StringVectorToList(entry->path));
    entry_dict.Set("curve", entry->curve);
    entries.Append(std::move(entry_dict));
  }
  dict.Set("entries", std::move(entries));
}

mojom::SnapSimpleConfigPtr DeserializeSimpleConfig(
    const base::DictValue& dict) {
  auto config = mojom::SnapSimpleConfig::New();
  ReadMaxRequestTimeFromStored(dict, config->max_request_time);
  return config;
}

mojom::SnapRpcConfigPtr DeserializeRpcConfig(const base::DictValue& dict) {
  auto config = mojom::SnapRpcConfig::New();
  config->allow_dapps = dict.FindBool("allow_dapps").value_or(false);
  config->allow_snaps = dict.FindBool("allow_snaps").value_or(false);
  ReadStringList(dict, "allowed_origins", config->allowed_origins);
  ReadMaxRequestTimeFromStored(dict, config->max_request_time);
  return config;
}

mojom::SnapNameLookupConfigPtr DeserializeNameLookupConfig(
    const base::DictValue& dict) {
  auto config = mojom::SnapNameLookupConfig::New();
  ReadStringList(dict, "chains", config->chains);
  if (const base::DictValue* matchers = dict.FindDict("matchers")) {
    auto matcher_config = mojom::SnapNameLookupMatchers::New();
    ReadStringList(*matchers, "tlds", matcher_config->tlds);
    ReadStringList(*matchers, "schemes", matcher_config->schemes);
    config->matchers = std::move(matcher_config);
  }
  ReadMaxRequestTimeFromStored(dict, config->max_request_time);
  return config;
}

mojom::SnapCronjobRequestPtr DeserializeCronjobRequest(
    const base::DictValue& dict) {
  auto request = mojom::SnapCronjobRequest::New();
  if (const std::string* method = dict.FindString("method")) {
    request->method = *method;
  }
  if (const base::Value* params = dict.Find("params")) {
    request->params = params->Clone();
  }
  return request;
}

mojom::SnapCronjobConfigPtr DeserializeCronjobConfig(
    const base::DictValue& dict) {
  auto config = mojom::SnapCronjobConfig::New();
  if (const base::ListValue* jobs = dict.FindList("jobs")) {
    for (const auto& job_value : *jobs) {
      if (!job_value.is_dict()) {
        continue;
      }
      const base::DictValue& job_dict = job_value.GetDict();
      auto job = mojom::SnapCronjob::New();
      if (const std::string* expression = job_dict.FindString("expression")) {
        job->expression = *expression;
      }
      if (const base::DictValue* request = job_dict.FindDict("request")) {
        job->request = DeserializeCronjobRequest(*request);
      }
      config->jobs.push_back(std::move(job));
    }
  }
  ReadMaxRequestTimeFromStored(dict, config->max_request_time);
  return config;
}

mojom::SnapBoolConfigPtr DeserializeBoolConfig(const base::DictValue& dict) {
  auto config = mojom::SnapBoolConfig::New();
  config->value = dict.FindBool("value").value_or(false);
  ReadMaxRequestTimeFromStored(dict, config->max_request_time);
  return config;
}

mojom::SnapKeyringConfigPtr DeserializeKeyringConfig(
    const base::DictValue& dict) {
  auto config = mojom::SnapKeyringConfig::New();
  ReadStringList(dict, "allowed_origins", config->allowed_origins);
  ReadMaxRequestTimeFromStored(dict, config->max_request_time);
  return config;
}

mojom::SnapBip44EntropyConfigPtr DeserializeBip44EntropyConfig(
    const base::DictValue& dict) {
  auto config = mojom::SnapBip44EntropyConfig::New();
  if (const base::ListValue* entries = dict.FindList("entries")) {
    for (const auto& item : *entries) {
      if (!item.is_dict()) {
        continue;
      }
      if (const std::optional<double> coin_type =
              item.GetDict().FindDouble("coin_type")) {
        auto entropy = mojom::SnapBip44Entropy::New();
        entropy->coin_type = static_cast<int32_t>(*coin_type);
        config->entries.push_back(std::move(entropy));
      }
    }
  }
  return config;
}

mojom::SnapBip32EntropyConfigPtr DeserializeBip32EntropyConfig(
    const base::DictValue& dict) {
  auto config = mojom::SnapBip32EntropyConfig::New();
  if (const base::ListValue* entries = dict.FindList("entries")) {
    for (const auto& item : *entries) {
      if (!item.is_dict()) {
        continue;
      }
      const base::DictValue& entry = item.GetDict();
      auto entropy = mojom::SnapBip32Entropy::New();
      ReadStringList(entry, "path", entropy->path);
      if (const std::string* curve = entry.FindString("curve")) {
        entropy->curve = *curve;
      }
      config->entries.push_back(std::move(entropy));
    }
  }
  return config;
}

void AppendPermissionIfSet(bool set,
                           const char* name,
                           std::vector<std::string>& out) {
  if (set) {
    out.push_back(name);
  }
}

}  // namespace

bool IsAllowedSnapPermission(std::string_view name) {
  for (const char* allowed : kAllowedPermissions) {
    if (name == allowed) {
      return true;
    }
  }
  return false;
}

void ApplySnapPermissionFromValue(const std::string& permission_name,
                                  const base::Value& value,
                                  mojom::SnapManifest& manifest) {
  if (permission_name == "endowment:network-access") {
    manifest.network_access = BuildSimpleConfig(value);
  } else if (permission_name == "endowment:rpc") {
    manifest.rpc = BuildRpcConfig(value);
  } else if (permission_name == "endowment:ethereum-provider") {
    manifest.ethereum_provider = BuildSimpleConfig(value);
  } else if (permission_name == "endowment:cronjob") {
    manifest.cronjob = BuildCronjobConfig(value);
  } else if (permission_name == "endowment:transaction-insight") {
    manifest.transaction_insight =
        BuildBoolConfig(value, "allowTransactionOrigin");
  } else if (permission_name == "endowment:signature-insight") {
    manifest.signature_insight =
        BuildBoolConfig(value, "allowSignatureOrigin");
  } else if (permission_name == "endowment:keyring") {
    manifest.keyring = BuildKeyringConfig(value);
  } else if (permission_name == "endowment:name-lookup") {
    manifest.name_lookup = BuildNameLookupConfig(value);
  } else if (permission_name == "endowment:page-home") {
    manifest.page_home = BuildSimpleConfig(value);
  } else if (permission_name == "endowment:lifecycle-hooks") {
    manifest.lifecycle_hooks = BuildSimpleConfig(value);
  } else if (permission_name == "endowment:webassembly") {
    manifest.webassembly = BuildSimpleConfig(value);
  } else if (permission_name == "snap_getBip44Entropy") {
    manifest.bip44_entropy = BuildBip44EntropyConfig(value);
  } else if (permission_name == "snap_getBip32Entropy") {
    manifest.bip32_entropy = BuildBip32EntropyConfig(value);
  } else if (permission_name == "snap_getEntropy") {
    manifest.get_entropy = BuildSimpleConfig(value);
  } else if (permission_name == "snap_dialog" ||
             permission_name == "snap_confirm") {
    manifest.dialog = BuildSimpleConfig(value);
  } else if (permission_name == "snap_notify") {
    manifest.notify = BuildSimpleConfig(value);
  } else if (permission_name == "snap_manageState") {
    manifest.manage_state = BuildSimpleConfig(value);
  }
}

std::vector<std::string> GetSnapPermissionNames(
    const mojom::SnapManifest& manifest) {
  std::vector<std::string> names;
  AppendPermissionIfSet(!!manifest.network_access, "endowment:network-access",
                        names);
  AppendPermissionIfSet(!!manifest.rpc, "endowment:rpc", names);
  AppendPermissionIfSet(!!manifest.ethereum_provider,
                        "endowment:ethereum-provider", names);
  AppendPermissionIfSet(!!manifest.cronjob, "endowment:cronjob", names);
  AppendPermissionIfSet(!!manifest.transaction_insight,
                        "endowment:transaction-insight", names);
  AppendPermissionIfSet(!!manifest.signature_insight,
                        "endowment:signature-insight", names);
  AppendPermissionIfSet(!!manifest.keyring, "endowment:keyring", names);
  AppendPermissionIfSet(!!manifest.name_lookup, "endowment:name-lookup", names);
  AppendPermissionIfSet(!!manifest.page_home, "endowment:page-home", names);
  AppendPermissionIfSet(!!manifest.lifecycle_hooks, "endowment:lifecycle-hooks",
                        names);
  AppendPermissionIfSet(!!manifest.webassembly, "endowment:webassembly", names);
  AppendPermissionIfSet(!!manifest.bip44_entropy, "snap_getBip44Entropy",
                        names);
  AppendPermissionIfSet(!!manifest.bip32_entropy, "snap_getBip32Entropy",
                        names);
  AppendPermissionIfSet(!!manifest.get_entropy, "snap_getEntropy", names);
  AppendPermissionIfSet(!!manifest.dialog, "snap_dialog", names);
  AppendPermissionIfSet(!!manifest.notify, "snap_notify", names);
  AppendPermissionIfSet(!!manifest.manage_state, "snap_manageState", names);
  return names;
}

bool SnapManifestHasPermission(const mojom::SnapManifest& manifest,
                               std::string_view name) {
  if (name == "snap_confirm") {
    return !!manifest.dialog;
  }
  if (name == "endowment:network-access") {
    return !!manifest.network_access;
  }
  if (name == "endowment:rpc") {
    return !!manifest.rpc;
  }
  if (name == "endowment:ethereum-provider") {
    return !!manifest.ethereum_provider;
  }
  if (name == "endowment:cronjob") {
    return !!manifest.cronjob;
  }
  if (name == "endowment:transaction-insight") {
    return !!manifest.transaction_insight;
  }
  if (name == "endowment:signature-insight") {
    return !!manifest.signature_insight;
  }
  if (name == "endowment:keyring") {
    return !!manifest.keyring;
  }
  if (name == "endowment:name-lookup") {
    return !!manifest.name_lookup;
  }
  if (name == "endowment:page-home") {
    return !!manifest.page_home;
  }
  if (name == "endowment:lifecycle-hooks") {
    return !!manifest.lifecycle_hooks;
  }
  if (name == "endowment:webassembly") {
    return !!manifest.webassembly;
  }
  if (name == "snap_getBip44Entropy") {
    return !!manifest.bip44_entropy;
  }
  if (name == "snap_getBip32Entropy") {
    return !!manifest.bip32_entropy;
  }
  if (name == "snap_getEntropy") {
    return !!manifest.get_entropy;
  }
  if (name == "snap_dialog") {
    return !!manifest.dialog;
  }
  if (name == "snap_notify") {
    return !!manifest.notify;
  }
  if (name == "snap_manageState") {
    return !!manifest.manage_state;
  }
  return false;
}

bool SnapManifestAllowsOrigin(const mojom::SnapManifest& manifest,
                              const url::Origin& origin) {
  if (!manifest.rpc) {
    return false;
  }
  if (manifest.rpc->allow_dapps) {
    return true;
  }
  if (manifest.rpc->allowed_origins.empty()) {
    return false;
  }
  const std::string serialized = origin.Serialize();
  return std::ranges::any_of(
      manifest.rpc->allowed_origins,
      [&serialized](const std::string& allowed_origin) {
        return allowed_origin == serialized;
      });
}

base::DictValue SnapManifestToValue(const mojom::SnapManifest& manifest) {
  base::DictValue dict;
  dict.Set("proposed_name", manifest.proposed_name);
  dict.Set("description", manifest.description);

  if (manifest.network_access) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.network_access, config);
    dict.Set("network_access", std::move(config));
  }
  if (manifest.rpc) {
    base::DictValue config;
    SerializeRpcConfig(*manifest.rpc, config);
    dict.Set("rpc", std::move(config));
  }
  if (manifest.ethereum_provider) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.ethereum_provider, config);
    dict.Set("ethereum_provider", std::move(config));
  }
  if (manifest.cronjob) {
    base::DictValue config;
    SerializeCronjobConfig(*manifest.cronjob, config);
    dict.Set("cronjob", std::move(config));
  }
  if (manifest.transaction_insight) {
    base::DictValue config;
    SerializeBoolConfig(*manifest.transaction_insight, config);
    dict.Set("transaction_insight", std::move(config));
  }
  if (manifest.signature_insight) {
    base::DictValue config;
    SerializeBoolConfig(*manifest.signature_insight, config);
    dict.Set("signature_insight", std::move(config));
  }
  if (manifest.keyring) {
    base::DictValue config;
    SerializeKeyringConfig(*manifest.keyring, config);
    dict.Set("keyring", std::move(config));
  }
  if (manifest.name_lookup) {
    base::DictValue config;
    SerializeNameLookupConfig(*manifest.name_lookup, config);
    dict.Set("name_lookup", std::move(config));
  }
  if (manifest.page_home) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.page_home, config);
    dict.Set("page_home", std::move(config));
  }
  if (manifest.lifecycle_hooks) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.lifecycle_hooks, config);
    dict.Set("lifecycle_hooks", std::move(config));
  }
  if (manifest.webassembly) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.webassembly, config);
    dict.Set("webassembly", std::move(config));
  }
  if (manifest.bip44_entropy) {
    base::DictValue config;
    SerializeBip44EntropyConfig(*manifest.bip44_entropy, config);
    dict.Set("bip44_entropy", std::move(config));
  }
  if (manifest.bip32_entropy) {
    base::DictValue config;
    SerializeBip32EntropyConfig(*manifest.bip32_entropy, config);
    dict.Set("bip32_entropy", std::move(config));
  }
  if (manifest.get_entropy) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.get_entropy, config);
    dict.Set("get_entropy", std::move(config));
  }
  if (manifest.dialog) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.dialog, config);
    dict.Set("dialog", std::move(config));
  }
  if (manifest.notify) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.notify, config);
    dict.Set("notify", std::move(config));
  }
  if (manifest.manage_state) {
    base::DictValue config;
    SerializeSimpleConfig(*manifest.manage_state, config);
    dict.Set("manage_state", std::move(config));
  }
  return dict;
}

mojom::SnapManifestPtr SnapManifestFromValue(const base::DictValue& dict) {
  auto manifest = mojom::SnapManifest::New();
  if (const std::string* v = dict.FindString("proposed_name")) {
    manifest->proposed_name = *v;
  }
  if (const std::string* v = dict.FindString("description")) {
    manifest->description = *v;
  }

  if (const base::DictValue* config = dict.FindDict("network_access")) {
    manifest->network_access = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("rpc")) {
    manifest->rpc = DeserializeRpcConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("ethereum_provider")) {
    manifest->ethereum_provider = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("cronjob")) {
    manifest->cronjob = DeserializeCronjobConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("transaction_insight")) {
    manifest->transaction_insight = DeserializeBoolConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("signature_insight")) {
    manifest->signature_insight = DeserializeBoolConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("keyring")) {
    manifest->keyring = DeserializeKeyringConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("name_lookup")) {
    manifest->name_lookup = DeserializeNameLookupConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("page_home")) {
    manifest->page_home = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("lifecycle_hooks")) {
    manifest->lifecycle_hooks = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("webassembly")) {
    manifest->webassembly = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("bip44_entropy")) {
    manifest->bip44_entropy = DeserializeBip44EntropyConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("bip32_entropy")) {
    manifest->bip32_entropy = DeserializeBip32EntropyConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("get_entropy")) {
    manifest->get_entropy = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("dialog")) {
    manifest->dialog = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("notify")) {
    manifest->notify = DeserializeSimpleConfig(*config);
  }
  if (const base::DictValue* config = dict.FindDict("manage_state")) {
    manifest->manage_state = DeserializeSimpleConfig(*config);
  }
  return manifest;
}

}  // namespace brave_wallet
