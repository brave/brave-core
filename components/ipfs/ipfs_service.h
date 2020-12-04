/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_H_

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"
#include "brave/components/ipfs/addresses_config.h"
#include "brave/components/ipfs/brave_ipfs_client_updater.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_p3a.h"
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
  using LaunchDaemonCallback = base::OnceCallback<void(bool)>;
  using ShutdownDaemonCallback = base::OnceCallback<void(bool)>;
  using GetConfigCallback = base::OnceCallback<void(bool, const std::string&)>;

  void AddObserver(IpfsServiceObserver* observer);
  void RemoveObserver(IpfsServiceObserver* observer);

  bool IsDaemonLaunched() const;
  static void RegisterPrefs(PrefRegistrySimple* registry);
  bool IsIPFSExecutableAvailable() const;
  void RegisterIpfsClientUpdater();
  IPFSResolveMethodTypes GetIPFSResolveMethodType() const;
  base::FilePath GetDataPath() const;
  base::FilePath GetConfigFilePath() const;

  // KeyedService
  void Shutdown() override;

  void GetConnectedPeers(GetConnectedPeersCallback callback);
  void GetAddressesConfig(GetAddressesConfigCallback callback);
  void LaunchDaemon(LaunchDaemonCallback callback);
  void ShutdownDaemon(ShutdownDaemonCallback callback);
  void GetConfig(GetConfigCallback);

  void SetIpfsLaunchedForTest(bool launched);
  void SetServerEndpointForTest(const GURL& gurl);
  void SetSkipGetConnectedPeersCallbackForTest(bool skip);
  void RunLaunchDaemonCallbackForTest(bool result);

 protected:
  base::FilePath GetIpfsExecutablePath();
  void OnConfigLoaded(GetConfigCallback, const std::pair<bool, std::string>&);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  // BraveIpfsClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;

  void OnIpfsCrashed();
  void OnIpfsLaunched(bool result, int64_t pid);
  void OnIpfsDaemonCrashed(int64_t pid);

  // Launches the ipfs service in an utility process.
  void LaunchIfNotRunning(const base::FilePath& executable_path);

  std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(const GURL& gurl);

  void OnGetConnectedPeers(SimpleURLLoaderList::iterator iter,
                           GetConnectedPeersCallback,
                           std::unique_ptr<std::string> response_body);
  void OnGetAddressesConfig(SimpleURLLoaderList::iterator iter,
                            GetAddressesConfigCallback callback,
                            std::unique_ptr<std::string> response_body);

  // The remote to the ipfs service running on an utility process. The browser
  // will not launch a new ipfs service process if this remote is already
  // bound.
  mojo::Remote<ipfs::mojom::IpfsService> ipfs_service_;

  int64_t ipfs_pid_ = -1;
  content::BrowserContext* context_;
  base::ObserverList<IpfsServiceObserver> observers_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;

  LaunchDaemonCallback launch_daemon_callback_;

  bool is_ipfs_launched_for_test_ = false;
  bool skip_get_connected_peers_callback_for_test_ = false;
  GURL server_endpoint_;

  base::FilePath user_data_dir_;
  BraveIpfsClientUpdater* ipfs_client_updater_;
  version_info::Channel channel_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  IpfsP3A ipfs_p3a;
  base::WeakPtrFactory<IpfsService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IpfsService);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_SERVICE_H_
