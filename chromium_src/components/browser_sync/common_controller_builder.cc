/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history/core/browser/sync/brave_history_delete_directives_model_type_controller.h"
#include "brave/components/history/core/browser/sync/brave_history_model_type_controller.h"

#define HistoryDeleteDirectivesDataTypeController \
  BraveHistoryDeleteDirectivesDataTypeController

#define HistoryDataTypeController BraveHistoryDataTypeController

#include "src/components/browser_sync/common_controller_builder.cc"

#undef HistoryDataTypeController
#undef HistoryDeleteDirectivesDataTypeController
