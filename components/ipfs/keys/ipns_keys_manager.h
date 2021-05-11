/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_KEYS_IPNS_KEYS_MANAGER_H_
#define BRAVE_COMPONENTS_IPFS_KEYS_IPNS_KEYS_MANAGER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace ipfs {

// Handles communication between browser and local node in order to generate,
// synchronize and remove p2p keys.
class IpnsKeysManager : public IpfsServiceObserver {
 public:
  IpnsKeysManager(content::BrowserContext* context,
                  const GURL& server_endpoint);
  ~IpnsKeysManager() override;

  IpnsKeysManager(const IpnsKeysManager&) = delete;
  IpnsKeysManager& operator=(const IpnsKeysManager&) = delete;

  using LoadKeysCallback = base::OnceCallback<void(bool)>;
  using GenerateKeyCallback =
      base::OnceCallback<void(bool, const std::string&, const std::string&)>;
  using RemoveKeyCallback = base::OnceCallback<void(const std::string&, bool)>;
  using KeysMap = std::unordered_map<std::string, std::string>;
  void LoadKeys(LoadKeysCallback callback);
  void GenerateNewKey(const std::string& name, GenerateKeyCallback callback);
  void RemoveKey(const std::string& name, RemoveKeyCallback callback);
  bool KeyExists(const std::string& name) const;

  const KeysMap& GetKeys() const { return keys_; }
  const std::string FindKey(const std::string& name) const;

  void SetServerEndpointForTest(const GURL& gurl);
  void SetLoadCallbackForTest(LoadKeysCallback callback);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  // ipfs::IpfsServiceObserver
  void OnIpfsLaunched(bool result, int64_t pid) override;
  void OnIpfsShutdown() override;

  void OnKeyCreated(SimpleURLLoaderList::iterator iter,
                    GenerateKeyCallback callback,
                    std::unique_ptr<std::string> response_body);
  void OnKeyRemoved(SimpleURLLoaderList::iterator iter,
                    const std::string& key_to_remove,
                    RemoveKeyCallback callback,
                    std::unique_ptr<std::string> response_body);

  void OnKeysLoaded(SimpleURLLoaderList::iterator iter,
                    std::unique_ptr<std::string> response_body);

  void NotifyKeysLoaded(bool result);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderList url_loaders_;
  std::unordered_map<std::string, std::string> keys_;
  base::queue<LoadKeysCallback> pending_load_callbacks_;
  content::BrowserContext* context_ = nullptr;
  GURL server_endpoint_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_KEYS_IPNS_KEYS_MANAGER_H_
