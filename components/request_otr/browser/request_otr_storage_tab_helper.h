/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_STORAGE_TAB_HELPER_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_STORAGE_TAB_HELPER_H_

#include "base/memory/ref_counted.h"
#include "brave/components/brave_shields/content/browser/blocked_domain_1pes_lifetime.h"
#include "content/public/browser/web_contents_user_data.h"

using brave_shields::BlockedDomain1PESLifetime;

namespace content {
class WebContents;
}  // namespace content

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace request_otr {

// Per-tab storage for Request-OTR interstitials, that stores a flag
// while proceeding so a new interstitial is not shown immediately,
// and stores the result of the user's request.
class RequestOTRStorageTabHelper
    : public content::WebContentsUserData<RequestOTRStorageTabHelper> {
 public:
  ~RequestOTRStorageTabHelper() override;

  // Disallow copy and assign.
  RequestOTRStorageTabHelper(const RequestOTRStorageTabHelper&) = delete;
  RequestOTRStorageTabHelper& operator=(const RequestOTRStorageTabHelper&) =
      delete;

  // Returns the RequestOTRStorageTabHelper associated to |web_contents|, or
  // creates one if there is none.
  static RequestOTRStorageTabHelper* GetOrCreate(
      content::WebContents* web_contents);

  void set_is_proceeding(bool is_proceeding) { is_proceeding_ = is_proceeding; }
  bool is_proceeding() const { return is_proceeding_; }
  void set_offered_otr(bool offered) { offered_ = offered; }
  bool has_offered_otr() { return offered_; }
  void set_requested_otr(bool otr);
  bool has_requested_otr() { return otr_; }

  void MaybeEnable1PESForUrl(
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      const GURL& url,
      base::OnceCallback<void(bool)> on_ready);

 private:
  explicit RequestOTRStorageTabHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<RequestOTRStorageTabHelper>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  void RecordSessionStats();

  // Flag stores whether we are in the middle of a proceed action.
  bool is_proceeding_ = false;
  // Flag stores whether we have offered going OTR for this tab.
  bool offered_ = false;
  // Flag stores whether the user requested OTR for this tab.
  bool otr_ = false;
  scoped_refptr<BlockedDomain1PESLifetime> blocked_domain_1pes_lifetime_;
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_STORAGE_TAB_HELPER_H_
