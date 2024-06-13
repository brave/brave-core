/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/credential_manager.h"
#include "brave/components/web_discovery/browser/double_fetcher.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "brave/components/web_discovery/browser/reporter.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
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

namespace web_discovery {

class WDPService : public KeyedService {
 public:
  WDPService(
      PrefService* local_state,
      PrefService* profile_prefs,
      base::FilePath user_data_dir,
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory);
  ~WDPService() override;

  WDPService(const WDPService&) = delete;
  WDPService& operator=(const WDPService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void OnFinishNavigation(const GURL& url,
                          content::RenderFrameHost* render_frame_host);

 private:
  void Start();
  void Stop();

  void OnEnabledChange();

  void OnConfigChange();
  void OnPatternsLoaded();
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

  RegexUtil regex_util_;

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  mojo::RemoteSet<mojom::DocumentExtractor> document_extractor_remotes_;

  std::unique_ptr<ServerConfigLoader> server_config_loader_;
  std::unique_ptr<CredentialManager> credential_manager_;
  std::unique_ptr<ContentScraper> content_scraper_;
  std::unique_ptr<DoubleFetcher> double_fetcher_;
  std::unique_ptr<Reporter> reporter_;

  base::Time current_page_count_start_time_;
  std::string current_page_count_hour_key_;
  base::RepeatingTimer alive_message_timer_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WDP_SERVICE_H_
