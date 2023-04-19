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

#include "base/containers/queue.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace network {
namespace mojom {
class URLLoaderFactory;
}
struct ResourceRequest;
}  // namespace network

namespace ipfs {

// Handles communication between browser and local node in order to generate,
// synchronize and remove p2p keys.
class IpnsKeysManager : public IpfsServiceObserver {
 public:
  IpnsKeysManager(
      BlobContextGetterFactory& blob_context_getter_factory,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const GURL& server_endpoint);
  ~IpnsKeysManager() override;

  IpnsKeysManager(const IpnsKeysManager&) = delete;
  IpnsKeysManager& operator=(const IpnsKeysManager&) = delete;

  using LoadKeysCallback = base::OnceCallback<void(bool)>;
  using GenerateKeyCallback =
      base::OnceCallback<void(bool, const std::string&, const std::string&)>;
  using RemoveKeyCallback = base::OnceCallback<void(const std::string&, bool)>;
  using ImportKeyCallback =
      base::OnceCallback<void(const std::string&, const std::string&, bool)>;
  using KeysMap = std::unordered_map<std::string, std::string>;
  void LoadKeys(LoadKeysCallback callback);
  void GenerateNewKey(const std::string& name, GenerateKeyCallback callback);
  void RemoveKey(const std::string& name, RemoveKeyCallback callback);
  bool KeyExists(const std::string& name) const;

  const KeysMap& GetKeys() const { return keys_; }
  const std::string FindKey(const std::string& name) const;

  void ImportKey(const base::FilePath& paths,
                 const std::string& name,
                 ImportKeyCallback callback);
  void SetServerEndpointForTest(const GURL& gurl);
  void SetLoadCallbackForTest(LoadKeysCallback callback);
  int GetLastLoadRetryForTest() const;
  void SetKeysForTest(
      const std::unordered_map<std::string, std::string>& keys) {
    keys_ = keys;
  }

 private:
  using APIRequestList =
      std::list<std::unique_ptr<api_request_helper::APIRequestHelper>>;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  // ipfs::IpfsServiceObserver
  void OnIpfsShutdown() override;
  void OnKeyImported(SimpleURLLoaderList::iterator iter,
                     ImportKeyCallback callback,
                     const std::string& key_name,
                     std::unique_ptr<std::string> response_body);
  void OnKeyCreated(APIRequestList::iterator iter,
                    GenerateKeyCallback callback,
                    api_request_helper::APIRequestResult response);
  void OnKeyRemoved(APIRequestList::iterator iter,
                    const std::string& key_to_remove,
                    RemoveKeyCallback callback,
                    api_request_helper::APIRequestResult response);
  void OnKeysLoaded(APIRequestList::iterator iter,
                    int retry_number,
                    api_request_helper::APIRequestResult response);

  void NotifyKeysLoaded(bool result);
  void LoadKeysInternal(int retries);
  void UploadData(ImportKeyCallback callback,
                  const std::string& name,
                  std::unique_ptr<network::ResourceRequest> request);

  int last_load_retry_value_for_test_ = -1;
  raw_ref<BlobContextGetterFactory> blob_context_getter_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  APIRequestList requests_list_;
  SimpleURLLoaderList url_loaders_;
  std::unordered_map<std::string, std::string> keys_;
  base::queue<LoadKeysCallback> pending_load_callbacks_;
  GURL server_endpoint_;
  base::WeakPtrFactory<IpnsKeysManager> weak_factory_{this};
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_KEYS_IPNS_KEYS_MANAGER_H_
