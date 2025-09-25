// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/services/brave_shields/filter_set_service.h"

#include "brave/components/brave_shields/core/browser/adblock/rs/src/lib.rs.h"

namespace brave_shields {

FilterSetService::FilterSetService(
    mojo::PendingReceiver<filter_set::mojom::UtilParseFilterSet> receiver)
    : receiver_(this, std::move(receiver)) {}

FilterSetService::~FilterSetService() = default;

void FilterSetService::ParseFilters(
    std::vector<filter_set::mojom::FilterListInputPtr> filters,
    ParseFiltersCallback callback) {
  auto filter_set = std::make_unique<rust::Box<adblock::FilterSet>>(
      adblock::new_filter_set());

  for (const auto& filter_list : filters) {
    auto metadata = (*filter_set)
                        ->add_filter_list_with_permissions(
                            filter_list->filters, filter_list->permission_mask);
    // TODO collect and return metadata
  }

  const auto e = adblock::engine_from_filter_set(std::move(*filter_set));
  CHECK(e.result_kind == adblock::ResultKind::Success);
  const auto dat = e.value->serialize();
  std::vector<unsigned char> output_dat;
  std::copy(dat.cbegin(), dat.cend(), std::back_inserter(output_dat));

  std::move(callback).Run(output_dat);
}

}  // namespace brave_shields
