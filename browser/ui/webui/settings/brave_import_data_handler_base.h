/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_DATA_HANDLER_BASE_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_DATA_HANDLER_BASE_H_

#include "chrome/browser/ui/webui/settings/import_data_handler.h"
#include "chrome/common/importer/importer_type.h"

namespace settings {

// This class is used for p3a. metric is repoated when importing is ended
// successfully.
class BraveImportDataHandlerBase : public ImportDataHandler {
 public:
  BraveImportDataHandlerBase();
  ~BraveImportDataHandlerBase() override;

  BraveImportDataHandlerBase(const BraveImportDataHandlerBase&) = delete;
  BraveImportDataHandlerBase& operator=(
      const BraveImportDataHandlerBase&) = delete;

 protected:
  // ImportDataHandler overrides:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t imported_items) override;
  void ImportEnded() override;

  importer::ImporterType type_ = importer::TYPE_UNKNOWN;
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_DATA_HANDLER_BASE_H_
