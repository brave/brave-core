// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/services/brave_shields/filter_parsing_service.h"

#include "base/containers/to_vector.h"
#include "base/logging.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"

namespace brave_shields {

FilterParsingService::FilterParsingService(
    mojo::PendingReceiver<
        adblock_filter_list_parser::mojom::AdblockFilterListParser> receiver)
    : receiver_(this, std::move(receiver)) {}

FilterParsingService::~FilterParsingService() = default;

void FilterParsingService::ParseFilters(
    std::vector<adblock_filter_list_parser::mojom::FilterListInputPtr> filters,
    ParseFiltersCallback callback) {
  auto filter_set = std::make_unique<rust::Box<adblock::FilterSet>>(
      adblock::new_filter_set());

  for (const auto& filter_list : filters) {
    auto metadata = (*filter_set)
                        ->add_filter_list_with_permissions(
                            filter_list->filters, filter_list->permission_mask);
    // TODO collect and return metadata
  }

  auto e = adblock::engine_from_filter_set(std::move(*filter_set));
  if (e.result_kind != adblock::ResultKind::Success) {
    VLOG(0) << "AdBlockEngine::OnFilterSetLoaded failed: "
            << e.error_message.c_str();
    return;
  }

  const auto dat = e.value->serialize();
  std::vector<unsigned char> output_dat = base::ToVector(dat);

  std::move(callback).Run(output_dat);
}

}  // namespace brave_shields
