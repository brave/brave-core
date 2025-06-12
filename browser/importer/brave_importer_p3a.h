/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_IMPORTER_P3A_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_IMPORTER_P3A_H_

#include "components/user_data_importer/common/importer_type.h"

// This mostly duplicates code in |importer_uma.cc| but we want better
// naming, better buckets and slightly different logic.
void RecordImporterP3A(user_data_importer::ImporterType type);

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_IMPORTER_P3A_H_
