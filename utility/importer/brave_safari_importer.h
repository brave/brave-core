/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_SAFARI_IMPORTER_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_SAFARI_IMPORTER_H_

#include "chrome/utility/importer/safari_importer.h"

class BraveSafariImporter : public SafariImporter {
 public:
  using SafariImporter::SafariImporter;

  BraveSafariImporter(const BraveSafariImporter&) = delete;
  BraveSafariImporter& operator=(const BraveSafariImporter&) = delete;

 private:
  // SafariImporter overrides:
  void ImportHistory() override;

  ~BraveSafariImporter() override;
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_SAFARI_IMPORTER_H_
