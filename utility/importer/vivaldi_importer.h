/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_IMPORTER_VIVALDI_IMPORTER_H_
#define BRAVE_UTILITY_IMPORTER_VIVALDI_IMPORTER_H_

#include "brave/utility/importer/chrome_importer.h"

class VivaldiImporter : public ChromeImporter {
 public:
  VivaldiImporter() = default;
  VivaldiImporter(const VivaldiImporter&) = delete;
  VivaldiImporter& operator=(const VivaldiImporter&) = delete;

 protected:
  ~VivaldiImporter() override {}
};

#endif  // BRAVE_UTILITY_IMPORTER_VIVALDI_IMPORTER_H_
