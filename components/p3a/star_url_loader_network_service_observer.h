/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_STAR_URL_LOADER_NETWORK_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_P3A_STAR_URL_LOADER_NETWORK_SERVICE_OBSERVER_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/network/public/mojom/url_loader_network_service_observer.mojom.h"

namespace p3a {

class StarURLLoaderNetworkServiceObserver
    : public network::mojom::URLLoaderNetworkServiceObserver {
 public:
  explicit StarURLLoaderNetworkServiceObserver(
      bool allow_unapproved_cert,
      base::RepeatingClosure approved_cert_mismatch_callback);
  ~StarURLLoaderNetworkServiceObserver() override;

  mojo::PendingRemote<network::mojom::URLLoaderNetworkServiceObserver> Bind();

  void SetApprovedCertFingerprint(
      std::optional<net::HashValue> approved_cert_fp);
  bool HasApprovedCert();

  void OnSSLCertificateError(const GURL& url,
                             int32_t net_error,
                             const net::SSLInfo& ssl_info,
                             bool fatal,
                             OnSSLCertificateErrorCallback callback) override;
  void OnCertificateRequested(
      const std::optional<base::UnguessableToken>& window_id,
      const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
      mojo::PendingRemote<network::mojom::ClientCertificateResponder>
          client_cert_responder) override;
  void OnAuthRequired(
      const std::optional<base::UnguessableToken>& window_id,
      uint32_t request_id,
      const GURL& url,
      bool first_auth_attempt,
      const net::AuthChallengeInfo& auth_info,
      const scoped_refptr<net::HttpResponseHeaders>& head_headers,
      mojo::PendingRemote<network::mojom::AuthChallengeResponder>
          auth_challenge_responder) override;
  void OnPrivateNetworkAccessPermissionRequired(
      const GURL& url,
      const net::IPAddress& ip_address,
      const std::optional<std::string>& private_network_device_id,
      const std::optional<std::string>& private_network_device_name,
      OnPrivateNetworkAccessPermissionRequiredCallback callback) override;
  void OnClearSiteData(
      const GURL& url,
      const std::string& header_value,
      int32_t load_flags,
      const std::optional<net::CookiePartitionKey>& cookie_partition_key,
      bool partitioned_state_allowed_only,
      OnClearSiteDataCallback callback) override;
  void OnLoadingStateUpdate(network::mojom::LoadInfoPtr info,
                            OnLoadingStateUpdateCallback callback) override;
  void OnDataUseUpdate(int32_t network_traffic_annotation_id_hash,
                       int64_t recv_bytes,
                       int64_t sent_bytes) override;
  void OnSharedStorageHeaderReceived(
      const url::Origin& request_origin,
      std::vector<network::mojom::SharedStorageOperationPtr> operations,
      OnSharedStorageHeaderReceivedCallback callback) override;
  void Clone(
      mojo::PendingReceiver<URLLoaderNetworkServiceObserver> observer) override;
  void OnWebSocketConnectedToPrivateNetwork(
      network::mojom::IPAddressSpace ip_address_space) override;

 private:
  bool allow_unapproved_cert_;
  base::RepeatingClosure approved_cert_mismatch_callback_;
  std::optional<net::HashValue> approved_cert_fp_;

  mojo::ReceiverSet<network::mojom::URLLoaderNetworkServiceObserver> receivers_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_STAR_URL_LOADER_NETWORK_SERVICE_OBSERVER_H_
