/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_INTERSTITIAL_CONTROLLER_CLIENT_H_
#define BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_INTERSTITIAL_CONTROLLER_CLIENT_H_

#include <memory>
#include <string>

#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace security_interstitials {
class MetricsHelper;
}  // namespace security_interstitials

class PrefService;

namespace unstoppable_domains {
enum class ResolveMethodTypes;

class UnstoppableDomainsInterstitialControllerClient
    : public security_interstitials::SecurityInterstitialControllerClient {
 public:
  static std::unique_ptr<security_interstitials::MetricsHelper>
  GetMetricsHelper(const GURL& url);

  UnstoppableDomainsInterstitialControllerClient(
      content::WebContents* web_contents,
      const GURL& request_url,
      PrefService* user_prefs,
      PrefService* local_state,
      const std::string& locale);
  ~UnstoppableDomainsInterstitialControllerClient() override = default;

  UnstoppableDomainsInterstitialControllerClient(
      const UnstoppableDomainsInterstitialControllerClient&) = delete;
  UnstoppableDomainsInterstitialControllerClient& operator=(
      const UnstoppableDomainsInterstitialControllerClient&) = delete;

  // security_interstitials::SecurityInterstitialControllerClient:
  void Proceed() override;
  void DontProceed();

 private:
  void SetResolveMethodAndReload(ResolveMethodTypes type);

  const GURL request_url_;
  PrefService* local_state_;
};

}  // namespace unstoppable_domains

#endif  // BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_INTERSTITIAL_CONTROLLER_CLIENT_H_
