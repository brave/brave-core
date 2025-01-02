// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_FILTER_LIST_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_FILTER_LIST_SERVICE_H_

#include <string>
#include <vector>
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/core/common/filter_list.mojom-forward.h"
#include "brave/components/brave_shields/core/common/filter_list.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace brave_shields {
class AdBlockService;

// This class is not thread-safe and should have single owner
class FilterListService : public KeyedService,
                          public mojom::FilterListAndroidHandler {
 public:
  explicit FilterListService(AdBlockService* ad_block_service);
  ~FilterListService() override;

  mojo::PendingRemote<mojom::FilterListAndroidHandler> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::FilterListAndroidHandler> receiver);

  void IsFilterListEnabled(const std::string& filterListUuid,
                           IsFilterListEnabledCallback callback) override;
  void EnableFilter(const std::string& filterListUuid,
                    bool shouldEnableFilter) override;
  void GetSubscriptions(GetSubscriptionsCallback callback) override;
  void CreateSubscription(const GURL& subscription_url) override;
  void EnableSubscription(const GURL& sub_url, bool enabled) override;
  void RefreshSubscription(const GURL& sub_url, bool from_ui) override;
  void DeleteSubscription(const GURL& sub_url) override;
  void GetFilterLists(GetFilterListsCallback callback) override;
  void GetCustomFilters(GetCustomFiltersCallback callback) override;
  void UpdateCustomFilters(const std::string& custom_filters,
                           UpdateCustomFiltersCallback callback) override;
  void UpdateFilterLists(UpdateFilterListsCallback callback) override;

 private:
  raw_ptr<AdBlockService> ad_block_service_ = nullptr;
  mojo::ReceiverSet<mojom::FilterListAndroidHandler> receivers_;
  base::WeakPtrFactory<FilterListService> weak_factory_{this};

  FilterListService(const FilterListService&) = delete;
  FilterListService& operator=(const FilterListService&) = delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_FILTER_LIST_SERVICE_H_
