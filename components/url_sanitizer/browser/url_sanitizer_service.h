/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_URL_SANITIZER_BROWSER_URL_SANITIZER_SERVICE_H_
#define BRAVE_COMPONENTS_URL_SANITIZER_BROWSER_URL_SANITIZER_SERVICE_H_

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "brave/components/url_sanitizer/common/mojom/url_sanitizer.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/common/url_pattern_set.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "mojo/public/cpp/bindings/receiver_set.h"
#endif  // # BUILDFLAG(IS_ANDROID)

namespace brave {

class URLSanitizerService : public KeyedService,
                            public URLSanitizerComponentInstaller::Observer,
                            public url_sanitizer::mojom::UrlSanitizerService {
 public:
  URLSanitizerService();
  ~URLSanitizerService() override;

  struct MatchItem {
    MatchItem();
    MatchItem(extensions::URLPatternSet include,
              extensions::URLPatternSet exclude,
              base::flat_set<std::string> params);
    MatchItem(MatchItem&&);
    ~MatchItem();

    extensions::URLPatternSet include;
    extensions::URLPatternSet exclude;
    base::flat_set<std::string> params;
  };

  struct Permissions {
    Permissions();
    Permissions(Permissions&&);
    ~Permissions();
    Permissions& operator=(Permissions&&);

    extensions::URLPatternSet js_api;
  };

  struct Config {
    Config();
    Config(Config&&);
    ~Config();
    Config& operator=(Config&&);

    std::vector<URLSanitizerService::MatchItem> matchers;
    Permissions permissions;
  };

#if BUILDFLAG(IS_ANDROID)
  mojo::PendingRemote<url_sanitizer::mojom::UrlSanitizerService> MakeRemote();
#endif  // # BUILDFLAG(IS_ANDROID)
  void SanitizeURL(const std::string& url,
                   SanitizeURLCallback callback) override;

  GURL SanitizeURL(const GURL& url);

  bool CheckJsPermission(const GURL& page_url);

  void SetInitializationCallbackForTesting(base::OnceClosure callback) {
    initialization_callback_for_testing_ = std::move(callback);
  }

 protected:
  friend class URLSanitizerServiceUnitTest;

  void UpdateConfig(Config config);

  std::string StripQueryParameter(const std::string& query,
                                  const base::flat_set<std::string>& trackers);

 private:
  // URLSanitizerComponentInstaller::Observer:
  void OnConfigReady(
      const URLSanitizerComponentInstaller::RawConfig& config) override;

  Config config_;

  base::OnceClosure initialization_callback_for_testing_;
#if BUILDFLAG(IS_ANDROID)
  mojo::ReceiverSet<url_sanitizer::mojom::UrlSanitizerService> receivers_;
#endif  // # BUILDFLAG(IS_ANDROID)
  base::WeakPtrFactory<URLSanitizerService> weak_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_URL_SANITIZER_BROWSER_URL_SANITIZER_SERVICE_H_
