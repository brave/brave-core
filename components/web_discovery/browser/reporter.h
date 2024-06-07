/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REPORTER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REPORTER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/web_discovery/browser/credential_manager.h"
#include "brave/components/web_discovery/browser/ecdh_aes.h"
#include "brave/components/web_discovery/browser/request_queue.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "net/http/http_response_headers.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace web_discovery {

class Reporter {
 public:
  Reporter(PrefService* profile_prefs,
           network::SharedURLLoaderFactory* shared_url_loader_factory,
           CredentialManager* credential_manager,
           std::unique_ptr<ServerConfig>* last_loaded_server_config);
  ~Reporter();

  Reporter(const Reporter&) = delete;
  Reporter& operator=(const Reporter&) = delete;

  void ScheduleSend(base::Value::Dict payload);

 private:
  void PrepareRequest(const base::Value& request_data);
  void OnRequestSigned(std::string final_payload_json,
                       uint8_t count_tag_hash,
                       size_t basename_count,
                       std::optional<std::vector<const uint8_t>> signature);
  void OnRequestCompressedAndEncrypted(uint8_t count_tag_hash,
                                       size_t basename_count,
                                       std::optional<AESEncryptResult> result);
  void OnRequestComplete(uint8_t count_tag_hash,
                         size_t basename_count,
                         scoped_refptr<net::HttpResponseHeaders> headers);
  bool ValidateResponse(scoped_refptr<net::HttpResponseHeaders> headers);

  GURL submit_url_;

  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<std::unique_ptr<ServerConfig>> last_loaded_server_config_;

  raw_ptr<CredentialManager> credential_manager_;

  scoped_refptr<base::SequencedTaskRunner> pool_sequenced_task_runner_;

  RequestQueue request_queue_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  base::WeakPtrFactory<Reporter> weak_ptr_factory_{this};
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_REPORTER_H_
