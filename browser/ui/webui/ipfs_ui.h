/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_IPFS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_IPFS_UI_H_

#include <memory>
#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace ipfs {
struct AddressesConfig;
struct RepoStats;
struct NodeInfo;
}  // namespace ipfs

class TestIPFSDomHandler;

class IPFSDOMHandler : public content::WebUIMessageHandler,
                       public ipfs::IpfsServiceObserver {
 public:
  IPFSDOMHandler();
  IPFSDOMHandler(const IPFSDOMHandler&) = delete;
  IPFSDOMHandler& operator=(const IPFSDOMHandler&) = delete;
  ~IPFSDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // ipfs::IpfsServiceObserver overrides:
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnIpfsShutdown() override;
  void OnGetConnectedPeers(bool success,
                           const std::vector<std::string>& peers) override;
  void OnInstallationEvent(ipfs::ComponentUpdaterEvents event) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(TestIPFSDomHandler, AddComponentVersion);
  FRIEND_TEST_ALL_PREFIXES(TestIPFSDomHandler, ComponentNotRegistered);
  void HandleGetConnectedPeers(const base::Value::List& args);
  void HandleGetAddressesConfig(const base::Value::List& args);
  void OnGetAddressesConfig(bool success,
                            const ipfs::AddressesConfig& config);
  void HandleGetDaemonStatus(const base::Value::List& args);
  void HandleLaunchDaemon(const base::Value::List& args);
  void LaunchDaemon();
  void OnLaunchDaemon(bool success);
  void HandleShutdownDaemon(const base::Value::List& args);
  void HandleRestartDaemon(const base::Value::List& args);
  void OnShutdownDaemon(bool success);
  void HandleGetRepoStats(const base::Value::List& args);
  void OnGetRepoStats(bool success, const ipfs::RepoStats& stats);
  void HandleGetNodeInfo(const base::Value::List& args);
  void OnGetNodeInfo(bool success, const ipfs::NodeInfo& info);

  void HandleGarbageCollection(const base::Value::List& args);
  void OnGarbageCollection(bool success, const std::string& error);

  void SetIpfsClientUpdaterVersionForTesting(const std::string& version) {
    client_updater_version_for_testing_ = version;
  }
  std::string GetIpfsClientUpdaterVersion() const;

  absl::optional<std::string> client_updater_version_for_testing_;
  base::ScopedObservation<ipfs::IpfsService, ipfs::IpfsServiceObserver>
      service_observer_{this};
  base::WeakPtrFactory<IPFSDOMHandler> weak_ptr_factory_;
};

// The WebUI for brave://ipfs
class IPFSUI : public content::WebUIController {
 public:
  IPFSUI(content::WebUI* web_ui, const std::string& host);
  IPFSUI(const IPFSUI&) = delete;
  IPFSUI& operator=(const IPFSUI&) = delete;
  ~IPFSUI() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_IPFS_UI_H_
