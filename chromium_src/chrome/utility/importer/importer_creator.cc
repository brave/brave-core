/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/utility/importer/importer_creator.h"
#define CreateImporterByType CreateImporterByType_ChromiumImpl
#include "../../../../../chrome/utility/importer/importer_creator.cc"
#undef CreateImporterByType

#include "brave/utility/importer/brave_importer.h"
#include "brave/utility/importer/chrome_importer.h"

namespace importer {

scoped_refptr<Importer> CreateImporterByType(ImporterType type) {
  switch (type) {
    case TYPE_CHROME:
      return new ChromeImporter();
    case TYPE_BRAVE:
      return new BraveImporter();
    default:
      return CreateImporterByType_ChromiumImpl(type);
  }
}

}  // namespace importer
