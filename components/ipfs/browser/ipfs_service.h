/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/ipfs/browser/brave_ipfs_client_updater.h"
#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace ipfs {

class IpfsService : public KeyedService,
                    public BraveIpfsClientUpdater::Observer {
 public:
  explicit IpfsService(content::BrowserContext* context);
  ~IpfsService() override;

  using GetConnectedPeersCallback = base::OnceCallback<
    void(bool,
         const std::vector<std::string>&)>;

  // KeyedService
  void Shutdown() override;

  void GetConnectedPeers(GetConnectedPeersCallback callback);

 protected:
  base::FilePath GetIpfsExecutablePath();

 private:
  // BraveIpfsClientUpdater::Observer
  void OnExecutableReady(const base::FilePath& path) override;

  void OnIpfsCrashed();
  void OnIpfsLaunched(bool result, int64_t pid);
  void OnIpfsDaemonCrashed(int64_t pid);

  // Launches the ipfs service in an utility process.
  void LaunchIfNotRunning(const base::FilePath& executable_path);

  void OnGetConnectedPeers(GetConnectedPeersCallback,
                           std::unique_ptr<std::string> response_body);

  // The remote to the ipfs service running on an utility process. The browser
  // will not launch a new ipfs service process if this remote is already
  // bound.
  mojo::Remote<ipfs::mojom::IpfsService> ipfs_service_;

  int64_t ipfs_pid_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  DISALLOW_COPY_AND_ASSIGN(IpfsService);
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_SERVICE_H_
