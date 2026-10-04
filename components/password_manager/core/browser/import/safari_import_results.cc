// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/password_manager/core/browser/import/safari_import_results.h"

namespace password_manager {

SafariImportEntry::SafariImportEntry() = default;
SafariImportEntry::SafariImportEntry(const SafariImportEntry& other) = default;
SafariImportEntry::SafariImportEntry(SafariImportEntry&& other) = default;
SafariImportEntry::~SafariImportEntry() = default;
SafariImportEntry& SafariImportEntry::operator=(
    const SafariImportEntry& entry) = default;
SafariImportEntry& SafariImportEntry::operator=(SafariImportEntry&& entry) =
    default;

SafariImportResults::SafariImportResults() = default;
SafariImportResults::SafariImportResults(const SafariImportResults& other) =
    default;
SafariImportResults::SafariImportResults(SafariImportResults&& other) = default;
SafariImportResults::~SafariImportResults() = default;
SafariImportResults& SafariImportResults::operator=(
    const SafariImportResults& entry) = default;
SafariImportResults& SafariImportResults::operator=(
    SafariImportResults&& entry) = default;

}  // namespace password_manager
