/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONTENT_ENS_OFFCHAIN_LOOKUP_INTERSTITIAL_CONTROLLER_CLIENT_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONTENT_ENS_OFFCHAIN_LOOKUP_INTERSTITIAL_CONTROLLER_CLIENT_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace security_interstitials {
class MetricsHelper;
}  // namespace security_interstitials

class PrefService;

namespace decentralized_dns {
enum class ResolveMethodTypes;

class EnsOffchainLookupInterstitialControllerClient
    : public security_interstitials::SecurityInterstitialControllerClient {
 public:
  static std::unique_ptr<security_interstitials::MetricsHelper>
  GetMetricsHelper(const GURL& url);

  EnsOffchainLookupInterstitialControllerClient(
      content::WebContents* web_contents,
      const GURL& request_url,
      PrefService* user_prefs,
      PrefService* local_state,
      const std::string& locale);
  ~EnsOffchainLookupInterstitialControllerClient() override = default;

  EnsOffchainLookupInterstitialControllerClient(
      const EnsOffchainLookupInterstitialControllerClient&) = delete;
  EnsOffchainLookupInterstitialControllerClient& operator=(
      const EnsOffchainLookupInterstitialControllerClient&) = delete;

  // security_interstitials::SecurityInterstitialControllerClient:
  void Proceed() override;
  void DontProceed();

 private:
  void SetResolveMethodAndReload(EnsOffchainResolveMethod type);

  const GURL request_url_;
  raw_ptr<PrefService> local_state_ = nullptr;
};

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_CONTENT_ENS_OFFCHAIN_LOOKUP_INTERSTITIAL_CONTROLLER_CLIENT_H_
