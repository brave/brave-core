/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_TAB_STORAGE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_TAB_STORAGE_H_

#include "base/memory/ref_counted.h"
#include "brave/components/brave_shields/browser/blocked_domain_1pes_lifetime.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace brave_shields {

// A short-lived, per tab storage for mixed form interstitials, that stores a
// flag while proceeding, so a new interstitial is not shown immediately.
class DomainBlockTabStorage
    : public content::WebContentsUserData<DomainBlockTabStorage> {
 public:
  ~DomainBlockTabStorage() override;

  // Disallow copy and assign.
  DomainBlockTabStorage(const DomainBlockTabStorage&) = delete;
  DomainBlockTabStorage& operator=(const DomainBlockTabStorage&) = delete;

  // Returns the DomainBlockTabStorage associated to |web_contents|, or
  // creates one if there is none.
  static DomainBlockTabStorage* GetOrCreate(content::WebContents* web_contents);

  void SetIsProceeding(bool is_proceeding) { is_proceeding_ = is_proceeding; }
  bool IsProceeding() const { return is_proceeding_; }

  void Enable1PESForUrlIfPossible(
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      const GURL& url,
      base::OnceCallback<void()> on_ready);
  void DropBlockedDomain1PESLifetime();

 private:
  explicit DomainBlockTabStorage(content::WebContents* contents);
  friend class content::WebContentsUserData<DomainBlockTabStorage>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();

  // Flag stores whether we are in the middle of a proceed action.
  bool is_proceeding_ = false;
  scoped_refptr<BlockedDomain1PESLifetime> blocked_domain_1pes_lifetime_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_TAB_STORAGE_H_
