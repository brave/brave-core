// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_PARSING_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_PARSING_SERVICE_H_

#include "brave/components/services/brave_shields/mojom/adblock_filter_list_parser.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_shields {

class FilterParsingService
    : public adblock_filter_list_parser::mojom::AdblockFilterListParser {
 public:
  explicit FilterParsingService(
      mojo::PendingReceiver<
          adblock_filter_list_parser::mojom::AdblockFilterListParser> receiver);
  FilterParsingService(const FilterParsingService&) = delete;
  FilterParsingService& operator=(const FilterParsingService&) = delete;
  ~FilterParsingService() override;

 private:
  void ParseFilters(
      std::vector<adblock_filter_list_parser::mojom::FilterListInputPtr>
          filters,
      ParseFiltersCallback callback) override;

  mojo::Receiver<adblock_filter_list_parser::mojom::AdblockFilterListParser>
      receiver_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_SHIELDS_FILTER_PARSING_SERVICE_H_
