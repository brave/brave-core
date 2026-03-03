// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/services/brave_shields/filter_parsing_service.h"

#include <memory>
#include <utility>

#include "base/containers/to_vector.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "brave/components/services/brave_shields/filter_parsing_service.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_shields {

namespace {
void BindInProcessFilterSetParser(
    mojo::PendingReceiver<
        adblock_filter_list_parser::mojom::AdblockFilterListParser> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<brave_shields::FilterParsingService>(),
      std::move(receiver));
}
}  // namespace

// static
mojo::PendingRemote<adblock_filter_list_parser::mojom::AdblockFilterListParser>
FilterParsingService::LaunchInProcessFilterParsingService() {
  mojo::PendingRemote<
      adblock_filter_list_parser::mojom::AdblockFilterListParser>
      remote;
  base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})
      ->PostTask(FROM_HERE,
                 base::BindOnce(&BindInProcessFilterSetParser,
                                remote.InitWithNewPipeAndPassReceiver()));
  return remote;
}

FilterParsingService::FilterParsingService() = default;

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

  std::vector<adblock_filter_list_parser::mojom::FilterListMetadataPtr>
      metadata;
  for (const auto& filter_list : filters) {
    base::span<const uint8_t> filter_data = filter_list->filters;
    std::vector<uint8_t> filter_vec(filter_data.begin(), filter_data.end());

    auto this_metadata = (*filter_set)
                             ->add_filter_list_with_permissions(
                                 filter_vec, filter_list->permission_mask);
    auto mojom_metadata =
        adblock_filter_list_parser::mojom::FilterListMetadata::New();
    if (this_metadata.result_kind == adblock::ResultKind::Success) {
      if (this_metadata.value.title.has_value) {
        mojom_metadata->title = this_metadata.value.title.value.c_str();
      }
      if (this_metadata.value.homepage.has_value) {
        mojom_metadata->homepage = this_metadata.value.homepage.value.c_str();
      }
      if (this_metadata.value.expires_hours.has_value) {
        mojom_metadata->expires_hours = this_metadata.value.expires_hours.value;
      }
    }
    metadata.push_back(std::move(mojom_metadata));
  }

  auto e = adblock::engine_from_filter_set(std::move(*filter_set));
  if (e.result_kind != adblock::ResultKind::Success) {
    VLOG(0) << "AdBlockEngine::OnFilterSetLoaded failed: "
            << e.error_message.c_str();
    return;
  }

  const auto dat = e.value->serialize();
  mojo_base::BigBuffer output_buffer(base::as_byte_span(dat));

  std::move(callback).Run(std::move(output_buffer), std::move(metadata));
}

}  // namespace brave_shields
