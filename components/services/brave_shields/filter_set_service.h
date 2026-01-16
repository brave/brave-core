// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_SET_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_SET_SERVICE_H_

#include "brave/components/services/brave_shields/mojom/filter_set.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_shields {

class FilterSetService : public filter_set::mojom::UtilParseFilterSet {
 public:
  explicit FilterSetService(
      mojo::PendingReceiver<filter_set::mojom::UtilParseFilterSet> receiver);
  FilterSetService(const FilterSetService&) = delete;
  FilterSetService& operator=(const FilterSetService&) = delete;
  ~FilterSetService() override;

 private:
  void ParseFilters(std::vector<filter_set::mojom::FilterListInputPtr> filters,
                    ParseFiltersCallback callback) override;

  mojo::Receiver<filter_set::mojom::UtilParseFilterSet> receiver_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_SET_SERVICE_H_
