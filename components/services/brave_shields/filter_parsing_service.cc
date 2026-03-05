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
#include "brave/components/services/brave_shields/mojom/adblock_filter_list_parser.mojom-forward.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_shields {

namespace {
void BindInProcessFilterSetParser(
    mojo::PendingReceiver<adblock::mojom::AdblockFilterListParser> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<brave_shields::FilterParsingService>(),
      std::move(receiver));
}

adblock::mojom::FilterListMetadataPtr ConvertToMojomFilterListMetadata(
    adblock::FilterListMetadataResult metadata) {
  auto mojom_metadata = adblock::mojom::FilterListMetadata::New();
  if (metadata.result_kind == adblock::ResultKind::Success) {
    if (metadata.value.title.has_value) {
      mojom_metadata->title = metadata.value.title.value.c_str();
    }
    if (metadata.value.homepage.has_value) {
      mojom_metadata->homepage = metadata.value.homepage.value.c_str();
    }
    if (metadata.value.expires_hours.has_value) {
      mojom_metadata->expires_hours = metadata.value.expires_hours.value;
    }
  }

  return mojom_metadata;
}
}  // namespace

// static
mojo::PendingRemote<adblock::mojom::AdblockFilterListParser>
FilterParsingService::LaunchInProcessFilterParsingService() {
  mojo::PendingRemote<adblock::mojom::AdblockFilterListParser> remote;
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
    mojo::PendingReceiver<adblock::mojom::AdblockFilterListParser> receiver)
    : receiver_(this, std::move(receiver)) {}

FilterParsingService::~FilterParsingService() = default;

void FilterParsingService::ParseFilters(
    std::vector<adblock::mojom::FilterListInputPtr> filters,
    ParseFiltersCallback callback) {
  auto filter_set = std::make_unique<rust::Box<adblock::FilterSet>>(
      adblock::new_filter_set());

  std::vector<adblock::mojom::FilterListMetadataPtr> metadata;
  for (auto& filter_list : filters) {
    auto filter_vec = base::ToVector(filter_list->filters.byte_span());

    auto this_metadata =
        (*filter_set)
            ->add_filter_list_with_permissions(std::move(filter_vec),
                                               filter_list->permission_mask);
    metadata.push_back(ConvertToMojomFilterListMetadata(this_metadata));
  }

  auto e = adblock::engine_from_filter_set(std::move(*filter_set));
  if (e.result_kind != adblock::ResultKind::Success) {
    VLOG(0) << "AdBlockEngine::OnFilterSetLoaded failed: "
            << e.error_message.c_str();
    std::move(callback).Run(mojo_base::BigBuffer(), std::move(metadata));
    return;
  }

  const auto dat = e.value->serialize();
  mojo_base::BigBuffer output_buffer(base::as_byte_span(dat));

  std::move(callback).Run(std::move(output_buffer), std::move(metadata));
}

}  // namespace brave_shields
