/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/keys/ipns_keys_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/rand_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_json_parser.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/mime_util.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"

namespace {

// Retry after some time If local node responded with error.
// The keys are often called immediately after startup
// and node initialization may take some time.
constexpr int kDefaultRetries = 5;

// Used to retry requests if we got error from ipfs node,
// it may fail requests sometimes right after launch,
// Actual value will be generated randomly in range
// (kMinimalRequestRetryIntervalMs,
//  kRequestsRetryRate*kMinimalRequestRetryIntervalMs)
const int kMinimalRequestRetryIntervalMs = 350;
const int kRequestsRetryRate = 3;

base::TimeDelta CalculateKeysRetryTime() {
  return base::Milliseconds(
      base::RandInt(kMinimalRequestRetryIntervalMs,
                    kRequestsRetryRate * kMinimalRequestRetryIntervalMs));
}

}  // namespace

namespace ipfs {

IpnsKeysManager::IpnsKeysManager(
    BlobContextGetterFactory& context_factory,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const GURL& server_endpoint)
    : blob_context_getter_factory_(context_factory),
      url_loader_factory_(url_loader_factory),
      server_endpoint_(server_endpoint) {}

IpnsKeysManager::~IpnsKeysManager() = default;

void IpnsKeysManager::ImportKey(const base::FilePath& upload_file_path,
                                const std::string& name,
                                ImportKeyCallback callback) {
  auto upload_callback =
      base::BindOnce(&IpnsKeysManager::UploadData, weak_factory_.GetWeakPtr(),
                     std::move(callback), name);
  auto filename = upload_file_path.BaseName().MaybeAsASCII();
  auto file_request_callback = base::BindOnce(
      &CreateRequestForFile, upload_file_path, &*blob_context_getter_factory_,
      ipfs::kFileMimeType, filename, std::move(upload_callback));
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&CalculateFileSize, upload_file_path),
      std::move(file_request_callback));
}

bool IpnsKeysManager::KeyExists(const std::string& name) const {
  return keys_.count(name);
}

void IpnsKeysManager::RemoveKey(const std::string& name,
                                RemoveKeyCallback callback) {
  if (!KeyExists(name)) {
    VLOG(1) << "Key " << name << " doesn't exist";
    if (callback)
      std::move(callback).Run(name, false);
    return;
  }

  auto generate_endpoint = server_endpoint_.Resolve(kAPIKeyRemoveEndpoint);
  GURL gurl =
      net::AppendQueryParameter(generate_endpoint, kArgQueryParam, name);

  auto url_loader = std::make_unique<api_request_helper::APIRequestHelper>(
      GetIpfsNetworkTrafficAnnotationTag(), url_loader_factory_);

  auto iter =
      requests_list_.insert(requests_list_.begin(), std::move(url_loader));
  iter->get()->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpnsKeysManager::OnKeyRemoved, weak_factory_.GetWeakPtr(),
                     iter, name, std::move(callback)),
      {{net::HttpRequestHeaders::kOrigin,
        url::Origin::Create(gurl).Serialize()}});
}

void IpnsKeysManager::OnKeyRemoved(
    APIRequestList::iterator iter,
    const std::string& key_to_remove,
    RemoveKeyCallback callback,
    api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();
  requests_list_.erase(iter);

  bool success = response.Is2XXResponseCode();
  std::unordered_map<std::string, std::string> removed_keys;
  success = success && IPFSJSONParser::GetParseKeysFromJSON(
                           response.value_body(), &removed_keys);
  if (success) {
    if (removed_keys.count(key_to_remove))
      keys_.erase(key_to_remove);
  } else {
    VLOG(1) << "Fail to generate new key, response code:" << response_code;
  }
  if (callback)
    std::move(callback).Run(key_to_remove, success);
}

void IpnsKeysManager::GenerateNewKey(const std::string& name,
                                     GenerateKeyCallback callback) {
  if (KeyExists(name)) {
    VLOG(1) << "Key " << name << " already exists";
    if (callback)
      std::move(callback).Run(true, name, keys_[name]);
    return;
  }

  auto generate_endpoint = server_endpoint_.Resolve(kAPIKeyGenerateEndpoint);
  GURL gurl =
      net::AppendQueryParameter(generate_endpoint, kArgQueryParam, name);

  auto url_loader = std::make_unique<api_request_helper::APIRequestHelper>(
      GetIpfsNetworkTrafficAnnotationTag(), url_loader_factory_);

  auto iter =
      requests_list_.insert(requests_list_.begin(), std::move(url_loader));
  iter->get()->Request(
      "POST", gurl, std::string(), std::string(),
      base::BindOnce(&IpnsKeysManager::OnKeyCreated, weak_factory_.GetWeakPtr(),
                     iter, std::move(callback)),
      {{net::HttpRequestHeaders::kOrigin,
        url::Origin::Create(gurl).Serialize()}});
}

void IpnsKeysManager::OnKeyCreated(
    APIRequestList::iterator iter,
    GenerateKeyCallback callback,
    api_request_helper::APIRequestResult response) {
  int response_code = response.response_code();
  requests_list_.erase(iter);

  std::string name;
  std::string value;
  bool success = response.Is2XXResponseCode();
  success = success && IPFSJSONParser::GetParseSingleKeyFromJSON(
                           response.value_body(), &name, &value);
  if (success) {
    keys_[name] = value;
  } else {
    VLOG(1) << "Fail to generate new key, response_code = " << response_code;
  }
  if (callback)
    std::move(callback).Run(success, name, value);
}

void IpnsKeysManager::LoadKeys(LoadKeysCallback callback) {
  if (!pending_load_callbacks_.empty()) {
    if (callback)
      pending_load_callbacks_.push(std::move(callback));
    return;
  }
  if (callback)
    pending_load_callbacks_.push(std::move(callback));

  LoadKeysInternal(kDefaultRetries);
}

void IpnsKeysManager::LoadKeysInternal(int retries) {
  auto url_loader = std::make_unique<api_request_helper::APIRequestHelper>(
      GetIpfsNetworkTrafficAnnotationTag(), url_loader_factory_);

  auto iter =
      requests_list_.insert(requests_list_.begin(), std::move(url_loader));
  auto url = server_endpoint_.Resolve(kAPIKeyListEndpoint);
  iter->get()->Request(
      "POST", url, std::string(), std::string(),
      base::BindOnce(&IpnsKeysManager::OnKeysLoaded, weak_factory_.GetWeakPtr(),
                     iter, retries),
      {{net::HttpRequestHeaders::kOrigin,
        url::Origin::Create(url).Serialize()}});
}

void IpnsKeysManager::UploadData(
    ImportKeyCallback callback,
    const std::string& name,
    std::unique_ptr<network::ResourceRequest> request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!request)
    return;

  auto generate_endpoint = server_endpoint_.Resolve(kAPIKeyImportEndpoint);
  GURL url = net::AppendQueryParameter(generate_endpoint, kArgQueryParam, name);
  auto url_loader = CreateURLLoader(url, "POST", std::move(request));

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&IpnsKeysManager::OnKeyImported,
                     weak_factory_.GetWeakPtr(), iter, std::move(callback),
                     name));
}
void IpnsKeysManager::OnKeyImported(
    SimpleURLLoaderList::iterator iter,
    ImportKeyCallback callback,
    const std::string& key_name,
    std::unique_ptr<std::string> response_body) {
  auto* url_loader = iter->get();
  int error_code = url_loader->NetError();
  int response_code = -1;
  if (url_loader->ResponseInfo() && url_loader->ResponseInfo()->headers)
    response_code = url_loader->ResponseInfo()->headers->response_code();
  url_loaders_.erase(iter);

  bool success = (error_code == net::OK && response_code == net::HTTP_OK);
  std::string name;
  std::string value;
  std::unordered_map<std::string, std::string> new_keys;

  // TODO(apaymyshev): actually here we parse json from internet with browser
  // main process.
  success = success && IPFSJSONParser::GetParseSingleKeyFromJSON(*response_body,
                                                                 &name, &value);
  if (success) {
    DCHECK_EQ(key_name, name) << "Key names should be equal";
    keys_[name] = value;
  } else {
    VLOG(1) << "Fail to import key, error_code = " << error_code
            << " response_code = " << response_code;
  }
  if (callback)
    std::move(callback).Run(key_name, value, success);
}

void IpnsKeysManager::OnIpfsShutdown() {
  keys_.clear();
}

void IpnsKeysManager::OnKeysLoaded(
    APIRequestList::iterator iter,
    int retry_number,
    api_request_helper::APIRequestResult response) {
  requests_list_.erase(iter);
  last_load_retry_value_for_test_ = retry_number;
  if (response.error_code() == net::ERR_CONNECTION_REFUSED && retry_number) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&IpnsKeysManager::LoadKeysInternal,
                       weak_factory_.GetWeakPtr(), retry_number - 1),
        CalculateKeysRetryTime());
    return;
  }

  bool success = response.Is2XXResponseCode();
  std::unordered_map<std::string, std::string> new_keys;
  success = success && IPFSJSONParser::GetParseKeysFromJSON(
                           response.value_body(), &new_keys);
  if (success) {
    keys_.swap(new_keys);
  } else {
    VLOG(1) << "Fail to load keys, response_code = "
            << response.response_code();
  }
  NotifyKeysLoaded(success);
}

int IpnsKeysManager::GetLastLoadRetryForTest() const {
  return last_load_retry_value_for_test_;
}

void IpnsKeysManager::SetLoadCallbackForTest(LoadKeysCallback callback) {
  if (callback)
    pending_load_callbacks_.push(std::move(callback));
}

void IpnsKeysManager::NotifyKeysLoaded(bool result) {
  while (!pending_load_callbacks_.empty()) {
    if (pending_load_callbacks_.front())
      std::move(pending_load_callbacks_.front()).Run(result);
    pending_load_callbacks_.pop();
  }
}

void IpnsKeysManager::SetServerEndpointForTest(const GURL& gurl) {
  server_endpoint_ = gurl;
}

const std::string IpnsKeysManager::FindKey(const std::string& name) const {
  auto it = keys_.find(name);
  if (it == keys_.end())
    return std::string();
  return it->second;
}

}  // namespace ipfs
