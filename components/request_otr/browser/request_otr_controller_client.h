/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_CONTROLLER_CLIENT_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_CONTROLLER_CLIENT_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/security_interstitials/content/security_interstitial_controller_client.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace security_interstitials {
class MetricsHelper;
}  // namespace security_interstitials

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace request_otr {

class RequestOTRControllerClient
    : public security_interstitials::SecurityInterstitialControllerClient {
 public:
  static std::unique_ptr<security_interstitials::MetricsHelper>
  GetMetricsHelper(const GURL& url);

  RequestOTRControllerClient(
      content::WebContents* web_contents,
      const GURL& request_url,
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      PrefService* prefs,
      const std::string& locale);
  ~RequestOTRControllerClient() override;

  RequestOTRControllerClient(const RequestOTRControllerClient&) = delete;
  RequestOTRControllerClient& operator=(const RequestOTRControllerClient&) =
      delete;

  void SetDontWarnAgain(bool value);
  void ProceedOTR();

  // security_interstitials::SecurityInterstitialControllerClient:
  void GoBack() override;
  void Proceed() override;

 private:
  void ReloadPage();

  const GURL request_url_;
  bool dont_warn_again_;
  raw_ptr<ephemeral_storage::EphemeralStorageService>
      ephemeral_storage_service_ = nullptr;  // not owned

  base::WeakPtrFactory<RequestOTRControllerClient> weak_ptr_factory_{this};
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_CONTROLLER_CLIENT_H_
