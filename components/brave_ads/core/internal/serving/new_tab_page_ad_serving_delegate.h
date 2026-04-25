/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_DELEGATE_H_

namespace brave_ads {

struct NewTabPageAdInfo;

class NewTabPageAdServingDelegate {
 public:
  // Invoked when an opportunity arises to serve a new tab page ad.
  virtual void OnOpportunityAroseToServeNewTabPageAd() {}

  // Invoked when a new tab page ad is served.
  virtual void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {}

  // Invoked when a new tab page ad fails to serve.
  virtual void OnFailedToServeNewTabPageAd() {}

 protected:
  virtual ~NewTabPageAdServingDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NEW_TAB_PAGE_AD_SERVING_DELEGATE_H_
