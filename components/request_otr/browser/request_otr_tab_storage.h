/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_TAB_STORAGE_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_TAB_STORAGE_H_

#include "base/memory/ref_counted.h"
#include "brave/components/brave_shields/browser/blocked_domain_1pes_lifetime.h"
#include "content/public/browser/web_contents_user_data.h"

using brave_shields::BlockedDomain1PESLifetime;

namespace content {
class WebContents;
}  // namespace content

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace request_otr {

// A short-lived, per tab storage for Request-OTR interstitials, that stores a
// flag while proceeding so a new interstitial is not shown immediately,
// and stores the result of the user's request.
class RequestOTRTabStorage
    : public content::WebContentsUserData<RequestOTRTabStorage> {
 public:
  ~RequestOTRTabStorage() override;

  // Disallow copy and assign.
  RequestOTRTabStorage(const RequestOTRTabStorage&) = delete;
  RequestOTRTabStorage& operator=(const RequestOTRTabStorage&) = delete;

  // Returns the RequestOTRTabStorage associated to |web_contents|, or
  // creates one if there is none.
  static RequestOTRTabStorage* GetOrCreate(content::WebContents* web_contents);

  void SetIsProceeding(bool is_proceeding) { is_proceeding_ = is_proceeding; }
  bool IsProceeding() const { return is_proceeding_; }
  void SetOfferedOTR(bool offered) { offered_ = offered; }
  bool OfferedOTR() { return offered_; }
  void SetRequestedOTR(bool otr) { otr_ = otr; }
  bool RequestedOTR() { return otr_; }

  void Enable1PESForUrlIfPossible(
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      const GURL& url,
      base::OnceCallback<void()> on_ready);
  void DropBlockedDomain1PESLifetime();

 private:
  explicit RequestOTRTabStorage(content::WebContents* contents);
  friend class content::WebContentsUserData<RequestOTRTabStorage>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  // Flag stores whether we are in the middle of a proceed action.
  bool is_proceeding_ = false;
  // Flag stores whether we have offered going OTR for this tab.
  bool offered_ = false;
  // Flag stores whether the user requested OTR for this tab.
  bool otr_ = false;
  scoped_refptr<BlockedDomain1PESLifetime> blocked_domain_1pes_lifetime_;
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_TAB_STORAGE_H_
