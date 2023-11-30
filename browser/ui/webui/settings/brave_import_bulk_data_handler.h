/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_BULK_DATA_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_BULK_DATA_HANDLER_H_

#include <memory>
#include <optional>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/settings/brave_import_data_handler.h"
#include "brave/browser/ui/webui/settings/brave_importer_observer.h"
#include "build/build_config.h"

namespace settings {

// This class handles bulk requests to import multiple profiles to
// new target Brave profiles.
class BraveImportBulkDataHandler : public BraveImportDataHandler {
 public:
  BraveImportBulkDataHandler();
  ~BraveImportBulkDataHandler() override;

  using ProfileReadyCallback = base::OnceCallback<void(Profile* profile)>;

  BraveImportBulkDataHandler(const BraveImportBulkDataHandler&) = delete;
  BraveImportBulkDataHandler& operator=(const BraveImportBulkDataHandler&) =
      delete;

 protected:
  void HandleImportDataBulk(const base::Value::List& args);

  std::optional<int> GetProfileIndex(
      const importer::SourceProfile& source_profile);

  void PrepareProfile(const std::u16string& name,
                      ProfileReadyCallback callback);

  void ProfileReadyForImport(const importer::SourceProfile& source_profile,
                             uint16_t imported_items,
                             Profile* profile);
  // BraveImportDataHandler
  void NotifyImportProgress(const importer::SourceProfile& source_profile,
                            const base::Value::Dict& info) override;
  void OnImportEnded(const importer::SourceProfile& source_profile) override;

  // SettingsPageUIHandler
  void RegisterMessages() override;

  // ImportDataHandler overrides:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t imported_items) override;

 private:
  base::flat_set<int> importing_profiles_;
  base::WeakPtrFactory<BraveImportBulkDataHandler> weak_factory_{this};
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_BULK_DATA_HANDLER_H_
