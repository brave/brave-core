/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/ephemeral_storage_partition.h"

#include "components/leveldb_proto/public/proto_database_provider.h"
#include "components/variations/net/variations_http_headers.h"
#include "content/browser/devtools/devtools_instrumentation.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_constants.h"
#include "net/url_request/url_request_context.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom.h"

namespace content {

EphemeralStoragePartition::EphemeralStoragePartition(
    BrowserContext* browser_context,
    StoragePartitionImpl* non_ephemeral_storage_partition,
    const base::FilePath relative_partition_path)
    : browser_context_(browser_context),
      non_ephemeral_storage_partition_(non_ephemeral_storage_partition),
      relative_partition_path_(relative_partition_path) {}

EphemeralStoragePartition::~EphemeralStoragePartition() {}

base::FilePath EphemeralStoragePartition::GetPath() {
  return non_ephemeral_storage_partition_->GetPath();
}

network::mojom::NetworkContext* EphemeralStoragePartition::GetNetworkContext() {
  if (!network_context_.is_bound())
    InitNetworkContext();
  return network_context_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
EphemeralStoragePartition::GetURLLoaderFactoryForBrowserProcess() {
  return non_ephemeral_storage_partition_
      ->GetURLLoaderFactoryForBrowserProcess();
}

scoped_refptr<network::SharedURLLoaderFactory> EphemeralStoragePartition::
    GetURLLoaderFactoryForBrowserProcessWithCORBEnabled() {
  return non_ephemeral_storage_partition_
      ->GetURLLoaderFactoryForBrowserProcessWithCORBEnabled();
}

std::unique_ptr<network::PendingSharedURLLoaderFactory>
EphemeralStoragePartition::GetURLLoaderFactoryForBrowserProcessIOThread() {
  return non_ephemeral_storage_partition_
      ->GetURLLoaderFactoryForBrowserProcessIOThread();
}

network::mojom::CookieManager*
EphemeralStoragePartition::GetCookieManagerForBrowserProcess() {
  // Create the CookieManager as needed.
  if (!cookie_manager_for_browser_process_ ||
      !cookie_manager_for_browser_process_.is_connected()) {
    // Reset |cookie_manager_for_browser_process_| before binding it again.
    cookie_manager_for_browser_process_.reset();
    GetNetworkContext()->GetCookieManager(
        cookie_manager_for_browser_process_.BindNewPipeAndPassReceiver());
  }
  return cookie_manager_for_browser_process_.get();
}

void EphemeralStoragePartition::CreateHasTrustTokensAnswerer(
    mojo::PendingReceiver<network::mojom::HasTrustTokensAnswerer> receiver,
    const url::Origin& top_frame_origin) {
  non_ephemeral_storage_partition_->CreateHasTrustTokensAnswerer(
      std::move(receiver), top_frame_origin);
}

storage::QuotaManager* EphemeralStoragePartition::GetQuotaManager() {
  return non_ephemeral_storage_partition_->GetQuotaManager();
}

AppCacheService* EphemeralStoragePartition::GetAppCacheService() {
  return non_ephemeral_storage_partition_->GetAppCacheService();
}

BackgroundSyncContext* EphemeralStoragePartition::GetBackgroundSyncContext() {
  return non_ephemeral_storage_partition_->GetBackgroundSyncContext();
}

storage::FileSystemContext* EphemeralStoragePartition::GetFileSystemContext() {
  return non_ephemeral_storage_partition_->GetFileSystemContext();
}

storage::DatabaseTracker* EphemeralStoragePartition::GetDatabaseTracker() {
  return non_ephemeral_storage_partition_->GetDatabaseTracker();
}

DOMStorageContext* EphemeralStoragePartition::GetDOMStorageContext() {
  return non_ephemeral_storage_partition_->GetDOMStorageContext();
}

storage::mojom::IndexedDBControl&
EphemeralStoragePartition::GetIndexedDBControl() {
  return non_ephemeral_storage_partition_->GetIndexedDBControl();
}

NativeFileSystemEntryFactory*
EphemeralStoragePartition::GetNativeFileSystemEntryFactory() {
  return non_ephemeral_storage_partition_->GetNativeFileSystemEntryFactory();
}

ServiceWorkerContext* EphemeralStoragePartition::GetServiceWorkerContext() {
  return non_ephemeral_storage_partition_->GetServiceWorkerContext();
}

DedicatedWorkerService* EphemeralStoragePartition::GetDedicatedWorkerService() {
  return non_ephemeral_storage_partition_->GetDedicatedWorkerService();
}

SharedWorkerService* EphemeralStoragePartition::GetSharedWorkerService() {
  return non_ephemeral_storage_partition_->GetSharedWorkerService();
}

CacheStorageContext* EphemeralStoragePartition::GetCacheStorageContext() {
  return non_ephemeral_storage_partition_->GetCacheStorageContext();
}

GeneratedCodeCacheContext*
EphemeralStoragePartition::GetGeneratedCodeCacheContext() {
  return non_ephemeral_storage_partition_->GetGeneratedCodeCacheContext();
}

DevToolsBackgroundServicesContext*
EphemeralStoragePartition::GetDevToolsBackgroundServicesContext() {
  return non_ephemeral_storage_partition_
      ->GetDevToolsBackgroundServicesContext();
}

ContentIndexContext* EphemeralStoragePartition::GetContentIndexContext() {
  return non_ephemeral_storage_partition_->GetContentIndexContext();
}

#if !defined(OS_ANDROID)
HostZoomMap* EphemeralStoragePartition::GetHostZoomMap() {
  return non_ephemeral_storage_partition_->GetHostZoomMap();
}

HostZoomLevelContext* EphemeralStoragePartition::GetHostZoomLevelContext() {
  return non_ephemeral_storage_partition_->GetHostZoomLevelContext();
}

ZoomLevelDelegate* EphemeralStoragePartition::GetZoomLevelDelegate() {
  return non_ephemeral_storage_partition_->GetZoomLevelDelegate();
}
#endif  // !defined(OS_ANDROID)

PlatformNotificationContext*
EphemeralStoragePartition::GetPlatformNotificationContext() {
  return non_ephemeral_storage_partition_->GetPlatformNotificationContext();
}

leveldb_proto::ProtoDatabaseProvider*
EphemeralStoragePartition::GetProtoDatabaseProvider() {
  return non_ephemeral_storage_partition_->GetProtoDatabaseProvider();
}

void EphemeralStoragePartition::SetProtoDatabaseProvider(
    std::unique_ptr<leveldb_proto::ProtoDatabaseProvider>
        optional_proto_db_provider) {
  non_ephemeral_storage_partition_->SetProtoDatabaseProvider(
      std::move(optional_proto_db_provider));
}

void EphemeralStoragePartition::ClearDataForOrigin(
    uint32_t remove_mask,
    uint32_t quota_storage_remove_mask,
    const GURL& storage_origin) {
  non_ephemeral_storage_partition_->ClearDataForOrigin(
      remove_mask, quota_storage_remove_mask, storage_origin);
}

void EphemeralStoragePartition::ClearData(uint32_t remove_mask,
                                          uint32_t quota_storage_remove_mask,
                                          const GURL& storage_origin,
                                          const base::Time begin,
                                          const base::Time end,
                                          base::OnceClosure callback) {
  non_ephemeral_storage_partition_->ClearData(
      remove_mask, quota_storage_remove_mask, storage_origin, begin, end,
      std::move(callback));
}

void EphemeralStoragePartition::ClearData(
    uint32_t remove_mask,
    uint32_t quota_storage_remove_mask,
    OriginMatcherFunction origin_matcher,
    network::mojom::CookieDeletionFilterPtr cookie_deletion_filter,
    bool perform_storage_cleanup,
    const base::Time begin,
    const base::Time end,
    base::OnceClosure callback) {
  non_ephemeral_storage_partition_->ClearData(
      remove_mask, quota_storage_remove_mask, origin_matcher,
      std::move(cookie_deletion_filter), perform_storage_cleanup, begin, end,
      std::move(callback));
}

void EphemeralStoragePartition::ClearCodeCaches(
    base::Time begin,
    base::Time end,
    const base::RepeatingCallback<bool(const GURL&)>& url_matcher,
    base::OnceClosure callback) {
  non_ephemeral_storage_partition_->ClearCodeCaches(begin, end, url_matcher,
                                                    std::move(callback));
}

void EphemeralStoragePartition::Flush() {
  non_ephemeral_storage_partition_->Flush();
}

void EphemeralStoragePartition::ResetURLLoaderFactories() {
  non_ephemeral_storage_partition_->ResetURLLoaderFactories();
}

void EphemeralStoragePartition::AddObserver(DataRemovalObserver* observer) {
  non_ephemeral_storage_partition_->AddObserver(observer);
}

void EphemeralStoragePartition::RemoveObserver(DataRemovalObserver* observer) {
  non_ephemeral_storage_partition_->RemoveObserver(observer);
}

void EphemeralStoragePartition::ClearBluetoothAllowedDevicesMapForTesting() {
  non_ephemeral_storage_partition_->ClearBluetoothAllowedDevicesMapForTesting();
}

void EphemeralStoragePartition::FlushNetworkInterfaceForTesting() {
  non_ephemeral_storage_partition_->FlushNetworkInterfaceForTesting();
}

void EphemeralStoragePartition::WaitForDeletionTasksForTesting() {
  non_ephemeral_storage_partition_->WaitForDeletionTasksForTesting();
}

void EphemeralStoragePartition::WaitForCodeCacheShutdownForTesting() {
  non_ephemeral_storage_partition_->WaitForCodeCacheShutdownForTesting();
}

void EphemeralStoragePartition::SetNetworkContextForTesting(
    mojo::PendingRemote<network::mojom::NetworkContext>
        network_context_remote) {
  non_ephemeral_storage_partition_->SetNetworkContextForTesting(
      std::move(network_context_remote));
}

void EphemeralStoragePartition::InitNetworkContext() {
  network::mojom::NetworkContextParamsPtr context_params =
      network::mojom::NetworkContextParams::New();
  network::mojom::CertVerifierCreationParamsPtr cert_verifier_creation_params =
      network::mojom::CertVerifierCreationParams::New();
  GetContentClient()->browser()->ConfigureNetworkContextParams(
      browser_context_, /* in_memory = */ true, relative_partition_path_,
      context_params.get(), cert_verifier_creation_params.get());
  devtools_instrumentation::ApplyNetworkContextParamsOverrides(
      browser_context_, context_params.get());
  DCHECK(!context_params->cert_verifier_params)
      << "|cert_verifier_params| should not be set in the NetworkContextParams,"
         "as they will be replaced with either the newly configured "
         "|cert_verifier_creation_params| or with a new pipe to the "
         "CertVerifierService.";

  context_params->cert_verifier_params =
      GetCertVerifierParams(std::move(cert_verifier_creation_params));

  // This mechanisms should be used only for legacy internal headers. You can
  // find a recommended alternative approach on URLRequest::cors_exempt_headers
  // at services/network/public/mojom/url_loader.mojom.
  context_params->cors_exempt_header_list.push_back(
      kCorsExemptPurposeHeaderName);
  context_params->cors_exempt_header_list.push_back(
      kCorsExemptRequestedWithHeaderName);
  variations::UpdateCorsExemptHeaderForVariations(context_params.get());

  network_context_.reset();
  GetNetworkService()->CreateNetworkContext(
      network_context_.BindNewPipeAndPassReceiver(), std::move(context_params));
  DCHECK(network_context_);

  network_context_client_receiver_.reset();
  network_context_->SetClient(
      network_context_client_receiver_.BindNewPipeAndPassRemote());
  network_context_.set_disconnect_handler(
      base::BindOnce(&EphemeralStoragePartition::InitNetworkContext,
                     weak_factory_.GetWeakPtr()));
}

void EphemeralStoragePartition::OnAuthRequired(
    const base::Optional<base::UnguessableToken>& window_id,
    int32_t process_id,
    int32_t routing_id,
    uint32_t request_id,
    const GURL& url,
    bool first_auth_attempt,
    const net::AuthChallengeInfo& auth_info,
    network::mojom::URLResponseHeadPtr head,
    mojo::PendingRemote<network::mojom::AuthChallengeResponder>
        auth_challenge_responder) {
  non_ephemeral_storage_partition_->OnAuthRequired(
      window_id, process_id, routing_id, request_id, url, first_auth_attempt,
      auth_info, std::move(head), std::move(auth_challenge_responder));
}

void EphemeralStoragePartition::OnCertificateRequested(
    const base::Optional<base::UnguessableToken>& window_id,
    int32_t process_id,
    int32_t routing_id,
    uint32_t request_id,
    const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
    mojo::PendingRemote<network::mojom::ClientCertificateResponder>
        cert_responder) {
  non_ephemeral_storage_partition_->OnCertificateRequested(
      window_id, process_id, routing_id, request_id, cert_info,
      std::move(cert_responder));
}

void EphemeralStoragePartition::OnSSLCertificateError(
    int32_t process_id,
    int32_t routing_id,
    const GURL& url,
    int net_error,
    const net::SSLInfo& ssl_info,
    bool fatal,
    OnSSLCertificateErrorCallback response) {
  non_ephemeral_storage_partition_->OnSSLCertificateError(
      process_id, routing_id, url, net_error, ssl_info, fatal,
      std::move(response));
}

void EphemeralStoragePartition::OnFileUploadRequested(
    int32_t process_id,
    bool async,
    const std::vector<base::FilePath>& file_paths,
    OnFileUploadRequestedCallback callback) {
  non_ephemeral_storage_partition_->OnFileUploadRequested(
      process_id, async, file_paths, std::move(callback));
}

void EphemeralStoragePartition::OnCanSendReportingReports(
    const std::vector<url::Origin>& origins,
    OnCanSendReportingReportsCallback callback) {
  non_ephemeral_storage_partition_->OnCanSendReportingReports(
      origins, std::move(callback));
}

void EphemeralStoragePartition::OnCanSendDomainReliabilityUpload(
    const GURL& origin,
    OnCanSendDomainReliabilityUploadCallback callback) {
  non_ephemeral_storage_partition_->OnCanSendDomainReliabilityUpload(
      origin, std::move(callback));
}

void EphemeralStoragePartition::OnClearSiteData(
    int32_t process_id,
    int32_t routing_id,
    const GURL& url,
    const std::string& header_value,
    int load_flags,
    OnClearSiteDataCallback callback) {
  non_ephemeral_storage_partition_->OnClearSiteData(process_id, routing_id, url,
                                                    header_value, load_flags,
                                                    std::move(callback));
}

#if defined(OS_ANDROID)
void EphemeralStoragePartition::OnGenerateHttpNegotiateAuthToken(
    const std::string& server_auth_token,
    bool can_delegate,
    const std::string& auth_negotiate_android_account_type,
    const std::string& spn,
    OnGenerateHttpNegotiateAuthTokenCallback callback) {
  non_ephemeral_storage_partition_->OnGenerateHttpNegotiateAuthToken(
      server_auth_token, can_delegate, auth_negotiate_android_account_type, spn,
      std::move(callback));
}
#endif

#if defined(OS_CHROMEOS)
void EphemeralStoragePartition::OnTrustAnchorUsed() {
  non_ephemeral_storage_partition_->OnTrustAnchorUsed();
}
#endif

void EphemeralStoragePartition::OnSCTReportReady(const std::string& cache_key) {
  non_ephemeral_storage_partition_->OnSCTReportReady(cache_key);
}

void EphemeralStoragePartition::CreateRestrictedCookieManagerForScript(
    const url::Origin& origin,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    int process_id,
    int routing_id,
    mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver,
    mojo::PendingRemote<network::mojom::CookieAccessObserver> cookie_observer) {
  network::mojom::RestrictedCookieManagerRole role =
      network::mojom::RestrictedCookieManagerRole::SCRIPT;
  if (!GetContentClient()->browser()->WillCreateRestrictedCookieManager(
          role, browser_context_, origin, site_for_cookies, top_frame_origin,
          /* is_service_worker = */ false, process_id, routing_id, &receiver)) {
    GetNetworkContext()->GetRestrictedCookieManager(
        std::move(receiver), role, origin, site_for_cookies, top_frame_origin,
        std::move(cookie_observer));
  }
}

}  // namespace content
