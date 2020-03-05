/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_import_data_handler_base.h"

#include "brave/browser/importer/brave_importer_p3a.h"

namespace settings {

BraveImportDataHandlerBase::BraveImportDataHandlerBase() = default;
BraveImportDataHandlerBase::~BraveImportDataHandlerBase() = default;

void BraveImportDataHandlerBase::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  ImportDataHandler::StartImport(source_profile, imported_items);

  type_ = source_profile.importer_type;
}

void BraveImportDataHandlerBase::ImportEnded() {
  ImportDataHandler::ImportEnded();

  if (!import_did_succeed_)
    return;

  DCHECK_NE(importer::TYPE_UNKNOWN, type_);
  RecordImporterP3A(type_);
}

}  // namespace settings
