/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_PUBIC_BROWSER_EPHEMERAL_STORAGE_PARTITION_H_
#define BRAVE_CONTENT_PUBIC_BROWSER_EPHEMERAL_STORAGE_PARTITION_H_

#include "content/public/browser/storage_partition.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace content {

class StoragePartitionImpl;

class EphemeralStoragePartition : public StoragePartition,
                                  public network::mojom::NetworkContextClient {
 public:
  EphemeralStoragePartition(
      BrowserContext* browser_context,
      StoragePartitionImpl* non_ephemeral_storage_partition,
      const base::FilePath relative_partition_path);
  EphemeralStoragePartition(const EphemeralStoragePartition&) = delete;
  EphemeralStoragePartition& operator=(const EphemeralStoragePartition&) =
      delete;
  ~EphemeralStoragePartition() override;

  void CreateRestrictedCookieManagerForScript(
      const url::Origin& origin,
      const net::SiteForCookies& site_for_cookies,
      const url::Origin& top_frame_origin,
      int process_id,
      int routing_id,
      mojo::PendingReceiver<network::mojom::RestrictedCookieManager> receiver,
      mojo::PendingRemote<network::mojom::CookieAccessObserver>
          cookie_observer);

  // StoragePartition interface.
  base::FilePath GetPath() override;
  network::mojom::NetworkContext* GetNetworkContext() override;
  scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactoryForBrowserProcess() override;
  scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactoryForBrowserProcessWithCORBEnabled() override;
  std::unique_ptr<network::PendingSharedURLLoaderFactory>
  GetURLLoaderFactoryForBrowserProcessIOThread() override;
  network::mojom::CookieManager* GetCookieManagerForBrowserProcess() override;
  void CreateHasTrustTokensAnswerer(
      mojo::PendingReceiver<network::mojom::HasTrustTokensAnswerer> receiver,
      const url::Origin& top_frame_origin) override;
  storage::QuotaManager* GetQuotaManager() override;
  AppCacheService* GetAppCacheService() override;
  BackgroundSyncContext* GetBackgroundSyncContext() override;
  storage::FileSystemContext* GetFileSystemContext() override;
  storage::DatabaseTracker* GetDatabaseTracker() override;
  DOMStorageContext* GetDOMStorageContext() override;
  storage::mojom::IndexedDBControl& GetIndexedDBControl() override;
  NativeFileSystemEntryFactory* GetNativeFileSystemEntryFactory() override;
  ServiceWorkerContext* GetServiceWorkerContext() override;
  DedicatedWorkerService* GetDedicatedWorkerService() override;
  SharedWorkerService* GetSharedWorkerService() override;
  CacheStorageContext* GetCacheStorageContext() override;
  GeneratedCodeCacheContext* GetGeneratedCodeCacheContext() override;
  DevToolsBackgroundServicesContext* GetDevToolsBackgroundServicesContext()
      override;
  ContentIndexContext* GetContentIndexContext() override;
#if !defined(OS_ANDROID)
  HostZoomMap* GetHostZoomMap() override;
  HostZoomLevelContext* GetHostZoomLevelContext() override;
  ZoomLevelDelegate* GetZoomLevelDelegate() override;
#endif  // !defined(OS_ANDROID)
  PlatformNotificationContext* GetPlatformNotificationContext() override;
  leveldb_proto::ProtoDatabaseProvider* GetProtoDatabaseProvider() override;
  void SetProtoDatabaseProvider(
      std::unique_ptr<leveldb_proto::ProtoDatabaseProvider>
          optional_proto_db_provider) override;
  void ClearDataForOrigin(uint32_t remove_mask,
                          uint32_t quota_storage_remove_mask,
                          const GURL& storage_origin) override;
  void ClearData(uint32_t remove_mask,
                 uint32_t quota_storage_remove_mask,
                 const GURL& storage_origin,
                 const base::Time begin,
                 const base::Time end,
                 base::OnceClosure callback) override;
  void ClearData(uint32_t remove_mask,
                 uint32_t quota_storage_remove_mask,
                 OriginMatcherFunction origin_matcher,
                 network::mojom::CookieDeletionFilterPtr cookie_deletion_filter,
                 bool perform_storage_cleanup,
                 const base::Time begin,
                 const base::Time end,
                 base::OnceClosure callback) override;
  void ClearCodeCaches(
      base::Time begin,
      base::Time end,
      const base::RepeatingCallback<bool(const GURL&)>& url_matcher,
      base::OnceClosure callback) override;
  void Flush() override;
  void ResetURLLoaderFactories() override;
  void AddObserver(DataRemovalObserver* observer) override;
  void RemoveObserver(DataRemovalObserver* observer) override;
  void ClearBluetoothAllowedDevicesMapForTesting() override;
  void FlushNetworkInterfaceForTesting() override;
  void WaitForDeletionTasksForTesting() override;
  void WaitForCodeCacheShutdownForTesting() override;
  void SetNetworkContextForTesting(
      mojo::PendingRemote<network::mojom::NetworkContext>
          network_context_remote) override;

  // network::mojom::NetworkContextClient interface.
  void OnAuthRequired(
      const base::Optional<base::UnguessableToken>& window_id,
      int32_t process_id,
      int32_t routing_id,
      uint32_t request_id,
      const GURL& url,
      bool first_auth_attempt,
      const net::AuthChallengeInfo& auth_info,
      network::mojom::URLResponseHeadPtr head,
      mojo::PendingRemote<network::mojom::AuthChallengeResponder>
          auth_challenge_responder) override;
  void OnCertificateRequested(
      const base::Optional<base::UnguessableToken>& window_id,
      int32_t process_id,
      int32_t routing_id,
      uint32_t request_id,
      const scoped_refptr<net::SSLCertRequestInfo>& cert_info,
      mojo::PendingRemote<network::mojom::ClientCertificateResponder>
          cert_responder) override;
  void OnSSLCertificateError(int32_t process_id,
                             int32_t routing_id,
                             const GURL& url,
                             int net_error,
                             const net::SSLInfo& ssl_info,
                             bool fatal,
                             OnSSLCertificateErrorCallback response) override;
  void OnFileUploadRequested(int32_t process_id,
                             bool async,
                             const std::vector<base::FilePath>& file_paths,
                             OnFileUploadRequestedCallback callback) override;
  void OnCanSendReportingReports(
      const std::vector<url::Origin>& origins,
      OnCanSendReportingReportsCallback callback) override;
  void OnCanSendDomainReliabilityUpload(
      const GURL& origin,
      OnCanSendDomainReliabilityUploadCallback callback) override;
  void OnClearSiteData(int32_t process_id,
                       int32_t routing_id,
                       const GURL& url,
                       const std::string& header_value,
                       int load_flags,
                       OnClearSiteDataCallback callback) override;
#if defined(OS_ANDROID)
  void OnGenerateHttpNegotiateAuthToken(
      const std::string& server_auth_token,
      bool can_delegate,
      const std::string& auth_negotiate_android_account_type,
      const std::string& spn,
      OnGenerateHttpNegotiateAuthTokenCallback callback) override;
#endif
#if defined(OS_CHROMEOS)
  void OnTrustAnchorUsed() override;
#endif
  void OnSCTReportReady(const std::string& cache_key) override;

 private:
  void InitNetworkContext();

  BrowserContext* browser_context_;
  StoragePartitionImpl* non_ephemeral_storage_partition_;
  const base::FilePath relative_partition_path_;

  // This is the NetworkContext used to
  // make requests for the StoragePartition. When the network service is
  // enabled, the underlying NetworkContext will be owned by the network
  // service. When it's disabled, the underlying NetworkContext may either be
  // provided by the embedder, or is created by the StoragePartition and owned
  // by |network_context_owner_|.
  mojo::Remote<network::mojom::NetworkContext> network_context_;

  mojo::Remote<network::mojom::CookieManager>
      cookie_manager_for_browser_process_;
  mojo::Receiver<network::mojom::NetworkContextClient>
      network_context_client_receiver_{this};

  base::WeakPtrFactory<EphemeralStoragePartition> weak_factory_{this};
};

}  // namespace content

#endif  // BRAVE_CONTENT_PUBIC_BROWSER_EPHEMERAL_STORAGE_PARTITION_H_
