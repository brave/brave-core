/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Can't avoid this injection because this code should be surrounded by
// ImporterBridge::NotifyStarted() and NotifyEnded() with other import logic.
#define BRAVE_START_IMPORT \
  if ((items & importer::HISTORY) && !cancelled()) {  \
    bridge_->NotifyItemStarted(importer::HISTORY);    \
    ImportHistory();                                  \
    bridge_->NotifyItemEnded(importer::HISTORY);      \
  }

#include "../../../../../chrome/utility/importer/safari_importer.mm"

#undef BRAVE_START_IMPORT

