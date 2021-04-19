/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/containers/queue.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "brave/components/ipfs/addresses_config.h"
#include "brave/components/ipfs/brave_ipfs_client_updater.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_p3a.h"
#include "brave/components/ipfs/node_info.h"
#include "brave/components/ipfs/repo_stats.h"
#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/version_info/channel.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class PrefRegistrySimple;

namespace ipfs {

class BraveIpfsClientUpdater;
class IpfsServiceDelegate;
class IpfsServiceObserver;
class IpfsImportWorkerBase;

class IpfsService : public KeyedService,
                    public BraveIpfsClientUpdater::Observer {
 public:
  IpfsService(content::BrowserContext* context,
              ipfs::BraveIpfsClientUpdater* ipfs_client_updater,
              const base::FilePath& user_data_dir,
              version_info::Channel channel);
  ~IpfsService() override;

  using GetConnectedPeersCallback =
      base::OnceCallback<void(bool, const std::vector<std::string>&)>;
  using GetAddressesConfigCallback =
      base::OnceCallback<void(bool, const ipfs::AddressesConfig&)>;
  using GetRepoStatsCallback =
      base::OnceCallback<void(bool, const ipfs::RepoStats&)>;
  using GetNodeInfoCallback =
      base::OnceCallback<void(bool, const ipfs::NodeInfo&)>;
  using GarbageCollectionCallback =
      base::OnceCallback<void(bool, const std::string&)>;

  using LaunchDaemonCallback = base::OnceCallback<void(bool)>;
  using ShutdownDaemonCallback = base::OnceCallback<void(bool)>;
  using GetConfigCallback = base::OnceCallback<void(bool, const std::string&)>;

  // Retry after some time If local node responded with error.
  // The connected peers are often called immediately after startup
  // and node initialization may take some time.
  static constexpr int kPeersDefaultRetries = 5;

  void AddObserver(IpfsServiceObserver* observer);
  void RemoveObserver(IpfsServiceObserver* observer);

  bool IsDaemonLaunched() const;
  static void RegisterPrefs(PrefRegistrySimple* registry);
  bool IsIPFSExecutableAvailable() const;
  void RegisterIpfsClientUpdater();
  IPFSResolveMethodTypes GetIPFSResolveMethodType() const;
  base::FilePath GetIpfsExecutablePath() const;
  base::FilePath GetDataPath() const;
  base::FilePath GetConfigFilePath() const;

  // KeyedService
  void Shutdown() override;

  void RestartDaemon();
  virtual void ImportFileToIpfs(const base::FilePath& path,
                                ipfs::ImportCompletedCallback callback);

  virtual void ImportDirectoryToIpfs(const base::FilePath& folder,
                                     ImportCompletedCallback callback);
  virtual void ImportLinkToIpfs(const GURL& url,
                                ImportCompletedCallback callback);
  virtual void ImportTextToIpfs(const std::string& text,
                                const std::string& host,
                                ImportCompletedCallback callback);
  virtual void PreWarmShareableLink(const GURL& url);

  void OnImportFinished(ipfs::ImportCompletedCallback callback,
                        size_t key,
                        const ipfs::ImportedData& data);
  void GetConnectedPeers(GetConnectedPeersCallback callback,
                         int retries = kPeersDefaultRetries);
  void GetAddressesConfig(GetAddressesConfigCallback callback);
  virtual void LaunchDaemon(LaunchDaemonCallback callback);
  void ShutdownDaemon(ShutdownDaemonCallback callback);
  void StartDaemonAndLaunch(base::OnceCallback<void(void)> callback);
  void GetConfig(GetConfigCallback);
  void GetRepoStats(GetRepoStatsCallback callback);
  void GetNodeInfo(GetNodeInfoCallback callback);
  void RunGarbageCollection(GarbageCollectionCallback callback);

  void SetAllowIpfsLaunchForTest(bool launched);
  void SetServerEndpointForTest(const GURL& gurl);
  void SetSkipGetConnectedPeersCallbackForTest(bool skip);
  bool WasConnectedPeersCalledForTest() const;
  void SetGetConnectedPeersCalledForTest(bool value);
  void RunLaunchDaemonCallbackForTest(bool result);
  int GetLastPeersRetryForTest() const;
  void SetZeroPeersDeltaForTest(bool value);

  void SetPreWarmCalbackForTesting(base::OnceClosure callback) {
    prewarm_callback_for_testing_ = std::move(callback);
  }

 protected:
  void OnConfigLoaded(GetConfigCallback, const std::pair<bool, std::string>&);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  // BraveIpfsClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;
  void OnInstallationEvent(ComponentUpdaterEvents event) override;

  void OnIpfsCrashed();
  void OnIpfsLaunched(bool result, int64_t pid);
  void OnIpfsDaemonCrashed(int64_t pid);
  // Notifies tasks waiting to start the service.
  void NotifyDaemonLaunchCallbacks(bool result);
  // Launches the ipfs service in an utility process.
  void LaunchIfNotRunning(const base::FilePath& executable_path);
  base::TimeDelta CalculatePeersRetryTime();
  std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
      const GURL& gurl,
      const std::string& method = "POST");

  void OnGetConnectedPeers(SimpleURLLoaderList::iterator iter,
                           GetConnectedPeersCallback,
                           int retries,
                           std::unique_ptr<std::string> response_body);
  void OnGetAddressesConfig(SimpleURLLoaderList::iterator iter,
                            GetAddressesConfigCallback callback,
                            std::unique_ptr<std::string> response_body);
  void OnRepoStats(SimpleURLLoaderList::iterator iter,
                   GetRepoStatsCallback callback,
                   std::unique_ptr<std::string> response_body);
  void OnNodeInfo(SimpleURLLoaderList::iterator iter,
                  GetNodeInfoCallback callback,
                  std::unique_ptr<std::string> response_body);
  void OnGarbageCollection(SimpleURLLoaderList::iterator iter,
                           GarbageCollectionCallback callback,
                           std::unique_ptr<std::string> response_body);
  void OnPreWarmComplete(SimpleURLLoaderList::iterator iter,
                         std::unique_ptr<std::string> response_body);
  std::string GetStorageSize();
  // The remote to the ipfs service running on an utility process. The browser
  // will not launch a new ipfs service process if this remote is already
  // bound.
  mojo::Remote<ipfs::mojom::IpfsService> ipfs_service_;

  int64_t ipfs_pid_ = -1;
  content::BrowserContext* context_;
  base::ObserverList<IpfsServiceObserver> observers_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;

  base::queue<LaunchDaemonCallback> pending_launch_callbacks_;

  bool allow_ipfs_launch_for_test_ = false;
  bool skip_get_connected_peers_callback_for_test_ = false;
  bool connected_peers_function_called_ = false;
  int last_peers_retry_value_for_test_ = -1;
  bool zero_peer_time_for_test_ = false;
  base::OnceClosure prewarm_callback_for_testing_;
  GURL server_endpoint_;

  // This member is used to guard public methods that mutate state.
  bool reentrancy_guard_ = false;

  base::FilePath user_data_dir_;
  BraveIpfsClientUpdater* ipfs_client_updater_;
  version_info::Channel channel_;
  std::unordered_map<size_t, std::unique_ptr<IpfsImportWorkerBase>> importers_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  IpfsP3A ipfs_p3a;
  base::WeakPtrFactory<IpfsService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IpfsService);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_H_
