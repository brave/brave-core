// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/browser/backup_results_service.h"

#include <utility>

namespace brave_search {

BackupResultsService::BackupResults::BackupResults(int final_status_code,
                                                   std::string html)
    : final_status_code(final_status_code), html(std::move(html)) {}

BackupResultsService::BackupResults::~BackupResults() = default;

}  // namespace brave_search
