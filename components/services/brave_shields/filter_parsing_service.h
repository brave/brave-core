// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_PARSING_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_PARSING_SERVICE_H_

#include "brave/components/services/brave_shields/mojom/adblock_filter_list_parser.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_shields {

class FilterParsingService : public adblock::mojom::AdblockFilterListParser {
 public:
  // For binding to an externally owned Receiver, like with
  // |mojo::MakeSelfOwnedReceiver()|.
  FilterParsingService();
  explicit FilterParsingService(
      mojo::PendingReceiver<adblock::mojom::AdblockFilterListParser> receiver);
  FilterParsingService(const FilterParsingService&) = delete;
  FilterParsingService& operator=(const FilterParsingService&) = delete;
  ~FilterParsingService() override;

  static mojo::PendingRemote<adblock::mojom::AdblockFilterListParser>
  LaunchInProcessFilterParsingService();

 private:
  void ParseFilters(std::vector<adblock::mojom::FilterListInputPtr> filters,
                    ParseFiltersCallback callback) override;

  mojo::Receiver<adblock::mojom::AdblockFilterListParser> receiver_{this};
};

using FilterParsingServiceFactory = base::RepeatingCallback<
    mojo::PendingRemote<adblock::mojom::AdblockFilterListParser>()>;

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_PARSING_SERVICE_H_
