/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_UTILITY_IMPORTER_SAFARI_IMPORTER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_UTILITY_IMPORTER_SAFARI_IMPORTER_H_

#define BRAVE_SAFARI_IMPORTER_H \
  friend class BraveSafariImporter; \
  virtual void ImportHistory() {}

#include "../../../../../chrome/utility/importer/safari_importer.h"

#undef BRAVE_SAFARI_IMPORTER_H
#undef ImportHistory

#endif  // BRAVE_CHROMIUM_SRC_CHROME_UTILITY_IMPORTER_SAFARI_IMPORTER_H_
