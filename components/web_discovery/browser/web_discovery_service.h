/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WEB_DISCOVERY_SERVICE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WEB_DISCOVERY_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/credential_manager.h"
#include "brave/components/web_discovery/browser/double_fetcher.h"
#include "brave/components/web_discovery/browser/reporter.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "brave/components/web_discovery/browser/url_extractor.h"
#include "brave/components/web_discovery/common/web_discovery.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefRegistrySimple;
class PrefService;

namespace content {
class RenderFrameHost;
}

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_search {
class BackupResultsService;
}  // namespace brave_search

namespace web_discovery {

// The main service for the native re-implementation of Web Discovery Project.
// Handles scraping and reporting of relevant pages for opted-in users.
class WebDiscoveryService : public KeyedService {
 public:
  WebDiscoveryService(
      PrefService* local_state,
      PrefService* profile_prefs,
      base::FilePath user_data_dir,
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory,
      brave_search::BackupResultsService* backup_results_service);
  ~WebDiscoveryService() override;

  WebDiscoveryService(const WebDiscoveryService&) = delete;
  WebDiscoveryService& operator=(const WebDiscoveryService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // KeyedService:
  void Shutdown() override;

  // Called by `WebDiscoveryTabHelper` to notify on a page load.
  bool ShouldExtractFromPage(const GURL& url,
                             content::RenderFrameHost* render_frame_host);
  void StartExtractingFromPage(
      const GURL& url,
      mojo::Remote<mojom::DocumentExtractor> document_extractor);

 private:
  friend class WebDiscoveryServiceTest;
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryServiceTest, ServiceStartsWhenEnabled);
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryServiceTest,
                           ServiceDoesNotStartWhenDisabled);
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryServiceTest,
                           ServiceDoesNotStartWhenDisabledByPolicy);
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryServiceTest,
                           ServiceRestartsWhenReEnabled);
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryServiceTest,
                           ServiceStopsOnDisabledPrefChange);
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryServiceTest,
                           ServiceStopsOnPolicyDisabledPrefChange);

  void Start();
  void Stop();
  void ClearPrefs();

  bool IsWebDiscoveryEnabled() const;

  void OnEnabledChange();

  void OnConfigChange();
  void OnPatternsLoaded();
  // See patterns.h for details on strict vs. normal scraping
  void OnContentScraped(bool is_strict,
                        std::unique_ptr<PageScrapeResult> result);
  void OnDoubleFetched(const GURL& url,
                       const base::Value& associated_data,
                       std::optional<std::string> response_body);

  bool UpdatePageCountStartTime();
  void MaybeSendAliveMessage();

  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  base::FilePath user_data_dir_;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  raw_ptr<brave_search::BackupResultsService> backup_results_service_;

  mojo::RemoteSet<mojom::DocumentExtractor> document_extractor_remotes_;

  std::unique_ptr<ServerConfigLoader> server_config_loader_;
  std::unique_ptr<CredentialManager> credential_manager_;
  std::unique_ptr<ContentScraper> content_scraper_;
  std::unique_ptr<DoubleFetcher> double_fetcher_;
  std::unique_ptr<Reporter> reporter_;
  std::unique_ptr<URLExtractor> url_extractor_;

  base::Time current_page_count_start_time_;
  std::string current_page_count_hour_key_;
  base::RepeatingTimer alive_message_timer_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WEB_DISCOVERY_SERVICE_H_
