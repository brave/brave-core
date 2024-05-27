/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/import/ipfs_import_controller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "brave/browser/ipfs/import/save_package_observer.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/import/imported_data.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_navigation_throttle.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/keys/ipns_keys_manager.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/channel_info.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "net/base/url_util.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace {

// Converts url to directory name:
// https://one.two/ -> one.two
// https://one.two/some/path -> one.two_some_path
std::string GetDirectoryNameForWebPageImport(const GURL& url) {
  if (url.path().empty() || url.path() == "/")
    return url.host();
  std::string content = url.host() + url.path();
  base::ReplaceSubstringsAfterOffset(&content, 0, "/", "_");
  return content;
}

// The index.html page is the most common name used for default pages
// if no other page is specified. We use it to open imported pages
// by shareable link automatically in browsers.
const char kDefaultHtmlPageName[] = "index.html";

// Message center notifier id for user notifications
const char kNotifierId[] = "service.ipfs";

// Imported shareable link should have filename parameter.
const char kImportFileNameParam[] = "filename";

std::u16string GetImportNotificationTitle(ipfs::ImportState state) {
  switch (state) {
    case ipfs::IPFS_IMPORT_SUCCESS:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_IPFS_IMPORT_NOTIFICATION_TITLE);
    case ipfs::IPFS_IMPORT_ERROR_REQUEST_EMPTY:
    case ipfs::IPFS_IMPORT_ERROR_ADD_FAILED:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_IPFS_IMPORT_ERROR_NOTIFICATION_TITLE);
    case ipfs::IPFS_IMPORT_ERROR_MKDIR_FAILED:
    case ipfs::IPFS_IMPORT_ERROR_MOVE_FAILED:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_IPFS_IMPORT_PARTLY_COMPLETED_NOTIFICATION_TITLE);
    default:
      NOTREACHED_IN_MIGRATION();
      break;
  }
  return std::u16string();
}

std::u16string GetImportNotificationBody(ipfs::ImportState state,
                                         const GURL& shareable_link) {
  switch (state) {
    case ipfs::IPFS_IMPORT_SUCCESS:
      return l10n_util::GetStringFUTF16(
          IDS_IPFS_IMPORT_NOTIFICATION_BODY,
          base::UTF8ToUTF16(shareable_link.spec()));
    case ipfs::IPFS_IMPORT_ERROR_REQUEST_EMPTY:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_IPFS_IMPORT_ERROR_NO_REQUEST_BODY);
    case ipfs::IPFS_IMPORT_ERROR_ADD_FAILED:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_IPFS_IMPORT_ERROR_ADD_FAILED_BODY);
    case ipfs::IPFS_IMPORT_ERROR_MKDIR_FAILED:
    case ipfs::IPFS_IMPORT_ERROR_MOVE_FAILED:
      return brave_l10n::GetLocalizedResourceUTF16String(
          IDS_IPFS_IMPORT_PARTLY_COMPLETED_NOTIFICATION_BODY);
    default:
      NOTREACHED_IN_MIGRATION();
      break;
  }
  return std::u16string();
}

std::unique_ptr<message_center::Notification> CreateMessageCenterNotification(
    const std::u16string& title,
    const std::u16string& body,
    const std::string& uuid,
    const GURL& link) {
  message_center::RichNotificationData notification_data;
  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = u" ";
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, uuid, title, body,
      ui::ImageModel(), std::u16string(), link,
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kNotifierId),
      notification_data, nullptr);

  return notification;
}

base::FilePath CreateTempDownloadDirectory(const std::string& subdir) {
  base::FilePath temp_dir_path;
  base::CreateNewTempDirectory(base::FilePath::StringType(), &temp_dir_path);
  base::FilePath web_package_dir = temp_dir_path.AppendASCII(subdir);
  if (base::CreateDirectory(web_package_dir))
    return web_package_dir;
  return base::FilePath();
}

}  // namespace

namespace ipfs {

IpfsImportController::~IpfsImportController() = default;

IpfsImportController::IpfsImportController(content::WebContents& web_contents)
    : web_contents_(web_contents),
      ipfs_service_(*ipfs::IpfsServiceFactory::GetForContext(
          web_contents.GetBrowserContext())),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {}

void IpfsImportController::ImportLinkToIpfs(const GURL& url) {
  ipfs_service_->ImportLinkToIpfs(
      url, base::BindOnce(&IpfsImportController::OnImportCompleted,
                          weak_ptr_factory_.GetWeakPtr()));
}

void IpfsImportController::ImportCurrentPageToIpfs() {
  if (!web_contents_->IsSavable()) {
    VLOG(1) << "Unable to save pages with mime type:"
            << web_contents_->GetContentsMimeType();
    return;
  }
  web_contents_->Stop();

  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&CreateTempDownloadDirectory,
                     GetDirectoryNameForWebPageImport(web_contents_->GetURL())),
      base::BindOnce(&IpfsImportController::SaveWebPage,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IpfsImportController::SaveWebPage(const base::FilePath& directory) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (directory.empty()) {
    VLOG(1) << "Unable to create temporary directory for import";
    return;
  }
  base::FilePath saved_main_file_path =
      directory.AppendASCII(kDefaultHtmlPageName);
  net::GenerateSafeFileName(web_contents_->GetContentsMimeType(), false,
                            &saved_main_file_path);
  base::FilePath saved_main_directory_path = saved_main_file_path.DirName();
  saved_main_directory_path = saved_main_directory_path.Append(
      saved_main_file_path.RemoveExtension().BaseName().value() +
      FILE_PATH_LITERAL("_files"));
  auto* download_manager =
      web_contents_->GetBrowserContext()->GetDownloadManager();
  save_package_observer_ = std::make_unique<SavePackageFinishedObserver>(
      download_manager, saved_main_file_path,
      base::BindOnce(&IpfsImportController::OnDownloadFinished,
                     weak_ptr_factory_.GetWeakPtr(), directory));
  web_contents_->SavePage(saved_main_file_path, saved_main_directory_path,
                          content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML);
}

bool IpfsImportController::HasInProgressDownload(download::DownloadItem* item) {
  if (!save_package_observer_ || !item)
    return false;
  return save_package_observer_->HasInProgressDownload(item);
}

void IpfsImportController::OnDownloadFinished(
    const base::FilePath& path,
    download::DownloadItem* download) {
  DCHECK(download);
  switch (download->GetState()) {
    case download::DownloadItem::COMPLETE:
      ipfs_service_->ImportDirectoryToIpfs(
          path, std::string(),
          base::BindOnce(&IpfsImportController::OnWebPageImportCompleted,
                         weak_ptr_factory_.GetWeakPtr(), path));
      break;
    case download::DownloadItem::CANCELLED:
      base::ThreadPool::PostTask(
          FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
          base::GetDeletePathRecursivelyCallback(path.DirName()));
      break;
    default:
      NOTREACHED_IN_MIGRATION();
  }

  save_package_observer_.reset();
}

void IpfsImportController::ImportDirectoryToIpfs(const base::FilePath& path,
                                                 const std::string& key) {
  ipfs_service_->ImportDirectoryToIpfs(
      path, key,
      base::BindOnce(&IpfsImportController::OnImportCompleted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IpfsImportController::ImportTextToIpfs(const std::string& text) {
  ipfs_service_->ImportTextToIpfs(
      text, web_contents_->GetURL().host(),
      base::BindOnce(&IpfsImportController::OnImportCompleted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void IpfsImportController::ImportFileToIpfs(const base::FilePath& path,
                                            const std::string& key) {
  ipfs_service_->ImportFileToIpfs(
      path, key,
      base::BindOnce(&IpfsImportController::OnImportCompleted,
                     weak_ptr_factory_.GetWeakPtr()));
}

GURL IpfsImportController::CreateAndCopyShareableLink(
    const ipfs::ImportedData& data) {
  if (data.hash.empty())
    return GURL();
  std::string ipfs = ipfs::kIPFSScheme + std::string("://") + data.hash;
  if (!data.published_key.empty()) {
    auto key = ipfs_service_->GetIpnsKeysManager()->FindKey(data.published_key);
    if (!key.empty()) {
      ipfs = ipfs::kIPNSScheme + std::string("://") + key;
    }
  }
  auto shareable_link = ipfs::ToPublicGatewayURL(
      GURL(ipfs),
      user_prefs::UserPrefs::Get(web_contents_->GetBrowserContext()));
  if (!shareable_link.is_valid())
    return GURL();
  if (!data.filename.empty())
    shareable_link = net::AppendQueryParameter(
        shareable_link, kImportFileNameParam, data.filename);
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteText(base::UTF8ToUTF16(shareable_link.spec()));
  ipfs_service_->PreWarmShareableLink(shareable_link);
  return shareable_link;
}

void IpfsImportController::OnWebPageImportCompleted(
    const base::FilePath& imported_direcory,
    const ipfs::ImportedData& data) {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::TaskPriority::BEST_EFFORT, base::MayBlock()},
      GetDeletePathRecursivelyCallback(imported_direcory.DirName()));
  OnImportCompleted(data);
}

void IpfsImportController::OnImportCompleted(const ipfs::ImportedData& data) {
  auto link = CreateAndCopyShareableLink(data);
#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
  if (!link.is_valid()) {
    // Open node diagnostic page if import failed
    link = GURL(kIPFSWebUIURL);
  }
#endif
  PushNotification(GetImportNotificationTitle(data.state),
                   GetImportNotificationBody(data.state, link), link);
  if (data.state == ipfs::IPFS_IMPORT_SUCCESS) {
    GURL url = ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
    content::OpenURLParams params(url, content::Referrer(),
                                  WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                  ui::PAGE_TRANSITION_LINK, false);
    web_contents_->OpenURL(params, /*navigation_handle_callback=*/{});
  }
}

void IpfsImportController::PushNotification(const std::u16string& title,
                                            const std::u16string& body,
                                            const GURL& link) {
  auto notification = CreateMessageCenterNotification(
      title, body, base::Uuid::GenerateRandomV4().AsLowercaseString(), link);
  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  auto* display_service = NotificationDisplayService::GetForProfile(profile);
  display_service->Display(NotificationHandler::Type::SEND_TAB_TO_SELF,
                           *notification, /*metadata=*/nullptr);
}

void IpfsImportController::FileSelected(const ui::SelectedFileInfo& file,
                                        int index,
                                        void* params) {
  switch (dialog_type_) {
    case ui::SelectFileDialog::SELECT_OPEN_FILE:
      ImportFileToIpfs(file.path(), dialog_key_);
      break;
    case ui::SelectFileDialog::SELECT_EXISTING_FOLDER:
      ImportDirectoryToIpfs(file.path(), dialog_key_);
      break;
    default:
      NOTREACHED_IN_MIGRATION()
          << "Only existing file or directory import supported";
      break;
  }
  dialog_type_ = ui::SelectFileDialog::SELECT_NONE;
  select_file_dialog_.reset();
  dialog_key_.clear();
}

void IpfsImportController::FileSelectionCanceled(void* params) {
  select_file_dialog_.reset();
  dialog_key_.clear();
}

void IpfsImportController::ShowImportDialog(ui::SelectFileDialog::Type type,
                                            const std::string& key) {
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(&*web_contents_));
  if (!select_file_dialog_) {
    VLOG(1) << "Import already in progress";
    return;
  }
  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  const base::FilePath directory = profile->last_selected_directory();
  gfx::NativeWindow parent_window = web_contents_->GetTopLevelNativeWindow();
  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths =
      ui::SelectFileDialog::FileTypeInfo::ANY_PATH_OR_URL;
  dialog_type_ = type;
  dialog_key_ = key;
  select_file_dialog_->SelectFile(type, std::u16string(), directory,
                                  &file_types, 0, base::FilePath::StringType(),
                                  parent_window, nullptr);
}

}  // namespace ipfs
