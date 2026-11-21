// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_SERVICE_H_
#define BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_SERVICE_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/storage_partition_config.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

class PrefService;

namespace brave {

// Runtime-only proxy overrides for a profile's storage partitions. All methods
// must run on the UI sequence. Off-the-record profiles have separate instances;
// Tor profiles are excluded. Overrides never bypass managed proxy settings.
// Off-the-record instances CHECK that partition configs are in-memory.
class StoragePartitionProxyService : public KeyedService {
 public:
  // Whether proxy settings controlled by an extension or the command line take
  // precedence over an override. Enterprise policy and supervised-user proxy
  // settings, and policy dynamic routing rules, always take precedence.
  enum class Precedence {
    kOverrideExtensionAndCommandLine,
    kYieldToExtensionAndCommandLine,
  };

  // Receives proxy errors for partitions whose override is in effect. Errors
  // for partitions using the inherited profile configuration are reported to
  // Chrome's proxy error handling instead. `partition_config` has no blob URL
  // fallback mode.
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPACScriptError(
        const content::StoragePartitionConfig& partition_config,
        int32_t line_number,
        const std::string& details) {}
    virtual void OnRequestMaybeFailedDueToProxySettings(
        const content::StoragePartitionConfig& partition_config,
        int32_t net_error) {}
  };

  StoragePartitionProxyService(PrefService& prefs,
                               PrefService& local_state,
                               bool is_off_the_record);
  ~StoragePartitionProxyService() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Overrides are keyed by partition domain, name, and in-memory flag. The blob
  // URL fallback mode is ignored because partitions that differ only in it
  // share their storage. Can be set before the partition exists. Updates
  // affect subsequent requests, not active connections. Callers supply a parsed
  // config and its annotation.
  void SetProxyConfig(const content::StoragePartitionConfig& partition_config,
                      const net::ProxyConfigWithAnnotation& config,
                      Precedence precedence);
  // Resumes inheriting the latest profile proxy configuration.
  void ClearProxyConfig(
      const content::StoragePartitionConfig& partition_config);
  // Returns the requested override, even when managed settings take precedence.
  std::optional<net::ProxyConfigWithAnnotation> GetProxyConfig(
      const content::StoragePartitionConfig& partition_config) const;

  // Called while configuring the network context of `partition_config`.
  void ConfigureNetworkContextParams(
      const content::StoragePartitionConfig& partition_config,
      network::mojom::NetworkContextParams* params);
  void FlushForTesting();

  // KeyedService:
  void Shutdown() override;

 private:
  class Context;

  struct Override {
    net::ProxyConfigWithAnnotation config;
    Precedence precedence;
  };

  // Ordered from lowest to highest precedence.
  enum class SettingsControl {
    kNone,
    kExtensionOrCommandLine,
    kPolicy,
  };

  // CHECKs that `partition_config` can belong to this service's profile and
  // returns it without a blob URL fallback mode.
  content::StoragePartitionConfig ValidatedKey(
      const content::StoragePartitionConfig& partition_config) const;

  SettingsControl GetSettingsControl() const;
  void OnSettingsChanged();
  void RemoveContext(Context* context);

  const raw_ref<PrefService> prefs_;
  const raw_ref<PrefService> local_state_;
  const bool is_off_the_record_;

  PrefChangeRegistrar registrar_;
  PrefChangeRegistrar local_state_registrar_;
  // Keys have no blob URL fallback mode.
  std::map<content::StoragePartitionConfig, Override> overrides_;
  std::vector<scoped_refptr<Context>> contexts_;
  base::ObserverList<Observer> observers_;
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<StoragePartitionProxyService> weak_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_STORAGE_PARTITION_PROXY_SERVICE_H_
