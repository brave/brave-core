// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"

#include "base/test/task_environment.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/services/brave_shields/mojom/filter_set.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

class FakeFilterSetService : public filter_set::mojom::UtilParseFilterSet {
 public:
  FakeFilterSetService() {}
  FakeFilterSetService(const FakeFilterSetService&) = delete;
  FakeFilterSetService& operator=(const FakeFilterSetService&) = delete;
  ~FakeFilterSetService() override {}

 private:
  void ParseFilters(std::vector<filter_set::mojom::FilterListInputPtr> filters,
                    ParseFiltersCallback callback) override {
    // no-op
  }
};

class FiltersProviderManagerTestObserver
    : public brave_shields::AdBlockFiltersProvider::Observer {
 public:
  FiltersProviderManagerTestObserver() = default;

  // AdBlockFiltersProvider::Observer
  void OnChanged(bool is_default_engine) override { changed_count += 1; }

  int changed_count = 0;
};

TEST(AdBlockFiltersProviderManagerTest, WaitUntilInitialized) {
  base::test::SingleThreadTaskEnvironment task_environment;

  FiltersProviderManagerTestObserver test_observer;

  FakeFilterSetService fake_service;

  mojo::Receiver<filter_set::mojom::UtilParseFilterSet> receiver{&fake_service};

  mojo::Remote<filter_set::mojom::UtilParseFilterSet> service;
  service.Bind(receiver.BindNewPipeAndPassRemote());
  brave_shields::AdBlockFiltersProviderManager m(std::move(service));
  m.AddObserver(&test_observer);

  brave_shields::TestFiltersProvider provider1("", true, 0);
  EXPECT_EQ(test_observer.changed_count, 0);
  provider1.RegisterAsSourceProvider(&m);
  EXPECT_EQ(test_observer.changed_count, 1);
  brave_shields::TestFiltersProvider provider2("", true, 0);
  EXPECT_EQ(test_observer.changed_count, 1);
  provider2.RegisterAsSourceProvider(&m);
  EXPECT_EQ(test_observer.changed_count, 2);
}
