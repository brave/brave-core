/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/star_url_loader_network_service_observer.h"

#include <utility>

#include "content/public/browser/browser_thread.h"

namespace p3a {

StarURLLoaderNetworkServiceObserver::StarURLLoaderNetworkServiceObserver(
    bool allow_unapproved_cert,
    base::RepeatingClosure approved_cert_mismatch_callback)
    : allow_unapproved_cert_(allow_unapproved_cert),
      approved_cert_mismatch_callback_(
          std::move(approved_cert_mismatch_callback)) {}

StarURLLoaderNetworkServiceObserver::~StarURLLoaderNetworkServiceObserver() =
    default;

mojo::PendingRemote<network::mojom::URLLoaderNetworkServiceObserver>
StarURLLoaderNetworkServiceObserver::Bind() {
  mojo::PendingRemote<network::mojom::URLLoaderNetworkServiceObserver> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void StarURLLoaderNetworkServiceObserver::SetApprovedCertFingerprint(
    std::optional<net::HashValue> approved_cert_fp) {
  approved_cert_fp_ = approved_cert_fp;
}

bool StarURLLoaderNetworkServiceObserver::HasApprovedCert() {
  return approved_cert_fp_.has_value();
}

void StarURLLoaderNetworkServiceObserver::OnSSLCertificateError(
    const GURL& url,
    int32_t net_error,
    const net::SSLInfo& ssl_info,
    bool fatal,
    OnSSLCertificateErrorCallback callback) {
  // star-randsrv-v2 uses a self-signed certificate. We should bypass
  // the certificate authority error if we allow unattested certificates
  // (only when performing the attestation process itself) or if the
  // certificate's fingerprint matches the one in the attestation document.
  if (net_error == net::ERR_CERT_AUTHORITY_INVALID) {
    if (allow_unapproved_cert_) {
      net_error = net::OK;
    } else if (ssl_info.cert) {
      auto cert_fp_hash =
          net::HashValue(ssl_info.cert->CalculateChainFingerprint256());
      if (approved_cert_fp_ && cert_fp_hash == approved_cert_fp_) {
        net_error = net::OK;
      } else {
        approved_cert_mismatch_callback_.Run();
        net_error = net::ERR_CERT_AUTHORITY_INVALID;
      }
    }
  }
  std::move(callback).Run(net_error);
}

void StarURLLoaderNetworkServiceObserver::OnCertificateRequested(
    const std::optional<base::UnguessableToken>& window_id,
    const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
    mojo::PendingRemote<network::mojom::ClientCertificateResponder>
        client_cert_responder) {
  mojo::Remote<network::mojom::ClientCertificateResponder> cert_responder(
      std::move(client_cert_responder));
  cert_responder->CancelRequest();
}

void StarURLLoaderNetworkServiceObserver::OnAuthRequired(
    const std::optional<base::UnguessableToken>& window_id,
    uint32_t request_id,
    const GURL& url,
    bool first_auth_attempt,
    const net::AuthChallengeInfo& auth_info,
    const scoped_refptr<net::HttpResponseHeaders>& head_headers,
    mojo::PendingRemote<network::mojom::AuthChallengeResponder>
        auth_challenge_responder) {
  mojo::Remote<network::mojom::AuthChallengeResponder>
      auth_challenge_responder_remote(std::move(auth_challenge_responder));
  auth_challenge_responder_remote->OnAuthCredentials(std::nullopt);
}

void StarURLLoaderNetworkServiceObserver::
    OnPrivateNetworkAccessPermissionRequired(
        const GURL& url,
        const net::IPAddress& ip_address,
        const std::optional<std::string>& private_network_device_id,
        const std::optional<std::string>& private_network_device_name,
        OnPrivateNetworkAccessPermissionRequiredCallback callback) {
  std::move(callback).Run(false);
}

void StarURLLoaderNetworkServiceObserver::OnClearSiteData(
    const GURL& url,
    const std::string& header_value,
    int32_t load_flags,
    const std::optional<net::CookiePartitionKey>& cookie_partition_key,
    bool partitioned_state_allowed_only,
    OnClearSiteDataCallback callback) {
  std::move(callback).Run();
}

void StarURLLoaderNetworkServiceObserver::OnLoadingStateUpdate(
    network::mojom::LoadInfoPtr info,
    OnLoadingStateUpdateCallback callback) {
  std::move(callback).Run();
}

void StarURLLoaderNetworkServiceObserver::OnDataUseUpdate(
    int32_t network_traffic_annotation_id_hash,
    int64_t recv_bytes,
    int64_t sent_bytes) {}

void StarURLLoaderNetworkServiceObserver::OnSharedStorageHeaderReceived(
    const url::Origin& request_origin,
    std::vector<network::mojom::SharedStorageOperationPtr> operations,
    OnSharedStorageHeaderReceivedCallback callback) {
  std::move(callback).Run();
}

void StarURLLoaderNetworkServiceObserver::Clone(
    mojo::PendingReceiver<URLLoaderNetworkServiceObserver> observer) {
  receivers_.Add(this, std::move(observer));
}

void StarURLLoaderNetworkServiceObserver::OnWebSocketConnectedToPrivateNetwork(
    network::mojom::IPAddressSpace ip_address_space) {}

}  // namespace p3a
