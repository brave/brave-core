/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IMPORT_IPFS_IMPORT_CONTROLLER_H_
#define BRAVE_BROWSER_IPFS_IMPORT_IPFS_IMPORT_CONTROLLER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/ipfs/ipfs_service.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class WebContents;
}  // namespace content

class SavePackageFinishedObserver;

namespace download {
class DownloadItem;
}

namespace ui {
class SelectFileDialog;
}

namespace ipfs {

struct ImportedData;
class IpfsService;

// Prepares data for IPFS import and does some interactions with user.
// Shows dialog to select file or folder for import and pushes notifications
// when import completed.
class IpfsImportController : public ui::SelectFileDialog::Listener {
 public:
  explicit IpfsImportController(content::WebContents& web_contents);
  ~IpfsImportController() override;

  IpfsImportController(const IpfsImportController&) = delete;
  IpfsImportController& operator=(const IpfsImportController&) = delete;

  void ImportLinkToIpfs(const GURL& url);
  void ImportTextToIpfs(const std::string& text);
  void ImportFileToIpfs(const base::FilePath& path, const std::string& key);
  void ImportDirectoryToIpfs(const base::FilePath& path,
                             const std::string& key);
  void ImportCurrentPageToIpfs();

  void ShowImportDialog(ui::SelectFileDialog::Type type,
                        const std::string& key);
  bool HasInProgressDownload(download::DownloadItem* item);

  void SetIpfsServiceForTesting(ipfs::IpfsService* service) {
    ipfs_service_ = *service;
  }
  void SkipSavePageForTesting(bool value) {
    skip_save_page_for_testing_ = value;
  }

 private:
  // ui::SelectFileDialog::Listener
  void FileSelected(const ui::SelectedFileInfo& file,
                    int index,
                    void* params) override;
  void FileSelectionCanceled(void* params) override;

  void OnDownloadFinished(const base::FilePath& path,
                          download::DownloadItem* download);

  void SaveWebPage(const base::FilePath& directory);
  void PushNotification(const std::u16string& title,
                        const std::u16string& body,
                        const GURL& link);
  GURL CreateAndCopyShareableLink(const ipfs::ImportedData& data);
  void OnImportCompleted(const ipfs::ImportedData& data);
  void OnWebPageImportCompleted(const base::FilePath& imported_direcory,
                                const ipfs::ImportedData& data);

  std::unique_ptr<SavePackageFinishedObserver> save_package_observer_;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  ui::SelectFileDialog::Type dialog_type_ = ui::SelectFileDialog::SELECT_NONE;
  std::string dialog_key_;

  const raw_ref<content::WebContents> web_contents_;
  raw_ref<ipfs::IpfsService> ipfs_service_;
  bool skip_save_page_for_testing_ = false;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::WeakPtrFactory<IpfsImportController> weak_ptr_factory_{this};
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IMPORT_IPFS_IMPORT_CONTROLLER_H_
