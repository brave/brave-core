/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_DATA_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_DATA_HANDLER_H_

#include <memory>
#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/settings/brave_importer_observer.h"
#include "build/build_config.h"
#include "chrome/browser/ui/webui/settings/import_data_handler.h"
#include "content/public/browser/web_contents_observer.h"

namespace settings {

// This class checks whether Brave has full disk access permission to import
// safari data on macOS. ImportDataHandler::StartImport() will be run after
// checking disk access permission. If Brave doesn't have that permission, this
// will launch tab modal dialog to notify users about this lack of permission.

// We should display tab modal dialog after import dialog is closed from webui.
// To do that, this observes web contents to launch dialog after import dialog
// closed. If dialog is launched right after notifying import failure,
// dialog will be closed immediately because tab modal dialog is closed with
// new navigation start and tab is newly loaded for closing webui import dialog.
// The reason why native tab modal dialog is used here is to avoid modifying
// upstream import html/js source code.
class BraveImportDataHandler : public ImportDataHandler,
                                      content::WebContentsObserver {
 public:
  BraveImportDataHandler();
  ~BraveImportDataHandler() override;

  BraveImportDataHandler(const BraveImportDataHandler&) = delete;
  BraveImportDataHandler& operator=(const BraveImportDataHandler&) = delete;

 protected:
  using ContinueImportCallback = base::OnceCallback<void()>;

  const importer::SourceProfile& GetSourceProfileAt(int browser_index);
  void HandleImportData(const base::Value::List& args);
  // ImportDataHandler overrides:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t imported_items) override;

  void StartImportImpl(const importer::SourceProfile& source_profile,
                       uint16_t imported_items,
                       Profile* profile);
  virtual void NotifyImportProgress(
      const importer::SourceProfile& source_profile,
      const base::Value::Dict& info);
  virtual void OnImportEnded(const importer::SourceProfile& source_profile);

  void OnStartImport(const importer::SourceProfile& source_profile,
                     uint16_t imported_items);
#if BUILDFLAG(IS_MAC)
  void CheckDiskAccess(uint16_t imported_items,
                       base::FilePath source_path,
                       importer::ImporterType importer_type,
                       ContinueImportCallback callback);
  void OnGetDiskAccessPermission(ContinueImportCallback callback,
                                 base::FilePath source_path,
                                 bool allowed);

  // content::WebContentsObserver overrides:
  void DidStopLoading() override;

  bool guide_dialog_is_requested_ = false;
#endif
 private:
  std::unordered_map<base::FilePath, std::unique_ptr<BraveImporterObserver>>
      import_observers_;
  base::WeakPtrFactory<BraveImportDataHandler> weak_factory_{this};
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_IMPORT_DATA_HANDLER_H_
