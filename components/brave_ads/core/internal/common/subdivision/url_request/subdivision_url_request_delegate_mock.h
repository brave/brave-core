/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_URL_REQUEST_SUBDIVISION_URL_REQUEST_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_URL_REQUEST_SUBDIVISION_URL_REQUEST_DELEGATE_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class SubdivisionUrlRequestDelegateMock : public SubdivisionUrlRequestDelegate {
 public:
  SubdivisionUrlRequestDelegateMock();

  SubdivisionUrlRequestDelegateMock(const SubdivisionUrlRequestDelegateMock&) =
      delete;
  SubdivisionUrlRequestDelegateMock& operator=(
      const SubdivisionUrlRequestDelegateMock&) = delete;

  ~SubdivisionUrlRequestDelegateMock() override;

  MOCK_METHOD(void, OnWillFetchSubdivision, (const base::Time fetch_at));

  MOCK_METHOD(void, OnDidFetchSubdivision, (const std::string& subdivision));

  MOCK_METHOD(void, OnFailedToFetchSubdivision, ());

  MOCK_METHOD(void,
              OnWillRetryFetchingSubdivision,
              (const base::Time retry_at));

  MOCK_METHOD(void, OnDidRetryFetchingSubdivision, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_URL_REQUEST_SUBDIVISION_URL_REQUEST_DELEGATE_MOCK_H_
