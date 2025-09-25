// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_SERVICES_FILTER_SET_FILTER_SET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_SERVICES_FILTER_SET_FILTER_SET_SERVICE_H_

#include "brave/components/brave_shields/core/services/filter_set/mojom/filter_set.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace filter_set_service {

class FilterSetService : public filter_set::mojom::UtilParseFilterSet {
 public:
  explicit FilterSetService(mojo::PendingReceiver<filter_set::mojom::UtilParseFilterSet> receiver);
  FilterSetService(const FilterSetService&) = delete;
  FilterSetService& operator=(const FilterSetService&) = delete;
  ~FilterSetService() override;
 private:
  void ParseFilters(std::vector<filter_set::mojom::FilterListInputPtr> filters, ParseFiltersCallback callback) override;

  mojo::Receiver<filter_set::mojom::UtilParseFilterSet> receiver_;
};

} // namespace filter_set_service

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_SERVICES_FILTER_SET_FILTER_SET_SERVICE_H_
