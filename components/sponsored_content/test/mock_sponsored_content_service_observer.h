/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_MOCK_SPONSORED_CONTENT_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_MOCK_SPONSORED_CONTENT_SERVICE_OBSERVER_H_

#include "base/files/file_path.h"
#include "base/types/optional_ref.h"
#include "base/values.h"
#include "brave/components/sponsored_content/sponsored_content_service_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace sponsored_content::test {

class MockSponsoredContentServiceObserver
    : public SponsoredContentServiceObserver {
 public:
  MockSponsoredContentServiceObserver();

  MockSponsoredContentServiceObserver(
      const MockSponsoredContentServiceObserver&) = delete;
  MockSponsoredContentServiceObserver& operator=(
      const MockSponsoredContentServiceObserver&) = delete;

  ~MockSponsoredContentServiceObserver() override;

  MOCK_METHOD(void,
              OnSponsoredNewTabTakeoverContentDidUpdate,
              (const base::FilePath&,
               base::optional_ref<const base::DictValue>),
              (override));
  MOCK_METHOD(void,
              OnSponsoredSiteContentDidUpdate,
              (const base::FilePath&,
               base::optional_ref<const base::DictValue>),
              (override));
  MOCK_METHOD(void,
              OnSponsoredConversionConfigContentDidUpdate,
              (const base::FilePath&,
               base::optional_ref<const base::DictValue>),
              (override));
  MOCK_METHOD(void, OnSponsoredContentComponentDidUnregister, (), (override));
};

}  // namespace sponsored_content::test

#endif  // BRAVE_COMPONENTS_SPONSORED_CONTENT_TEST_MOCK_SPONSORED_CONTENT_SERVICE_OBSERVER_H_
