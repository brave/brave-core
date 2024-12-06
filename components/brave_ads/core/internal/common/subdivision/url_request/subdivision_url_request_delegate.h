/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_URL_REQUEST_SUBDIVISION_URL_REQUEST_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_URL_REQUEST_SUBDIVISION_URL_REQUEST_DELEGATE_H_

#include <string>

#include "base/time/time.h"

namespace brave_ads {

class SubdivisionUrlRequestDelegate {
 public:
  // Invoked to tell the delegate we will fetch the subdivision at `fetch_at`.
  virtual void OnWillFetchSubdivision(base::Time fetch_at) {}

  // Invoked to tell the delegate we successfully fetched the `subdivision`.
  virtual void OnDidFetchSubdivision(const std::string& subdvision) {}

  // Invoked to tell the delegate we failed to fetch the subdivision.
  virtual void OnFailedToFetchSubdivision() {}

  // Invoked to tell the delegate we will retry fetching the subdivision at
  // `retry_at`.
  virtual void OnWillRetryFetchingSubdivision(base::Time retry_at) {}

  // Invoked to tell the delegate we retried fetching the subdivision.
  virtual void OnDidRetryFetchingSubdivision() {}

 protected:
  virtual ~SubdivisionUrlRequestDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_URL_REQUEST_SUBDIVISION_URL_REQUEST_DELEGATE_H_
