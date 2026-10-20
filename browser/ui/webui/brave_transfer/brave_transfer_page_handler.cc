// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_transfer/brave_transfer_page_handler.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/select_file_policy/chrome_select_file_policy.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/shell_dialogs/selected_file_info.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_transfer {

BraveTransferPageHandler::BraveTransferPageHandler(
    mojo::PendingReceiver<mojom::PageHandler> receiver,
    mojo::PendingRemote<mojom::Page> page,
    BraveTransferService* service,
    content::WebContents* web_contents)
    : receiver_(this, std::move(receiver)),
      page_(std::move(page)),
      service_(service),
      web_contents_(web_contents) {
  CHECK(service_);
  service_observation_.Observe(service_);
  service_->AddPage();
}

BraveTransferPageHandler::~BraveTransferPageHandler() {
  if (select_file_dialog_) {
    select_file_dialog_->ListenerDestroyed();
  }
  service_observation_.Reset();
  service_->RemovePage();
}

void BraveTransferPageHandler::GetServiceState(
    GetServiceStateCallback callback) {
  std::move(callback).Run(service_->GetServiceState());
}

void BraveTransferPageHandler::OpenSyncSettings() {
  ShowSingletonTab(GetProfile(),
                   GURL(chrome::kChromeUISettingsURL).Resolve(kBraveSyncPath));
}

void BraveTransferPageHandler::GetDevices(GetDevicesCallback callback) {
  std::move(callback).Run(service_->GetDevices());
}

void BraveTransferPageHandler::SetDevicePolicy(
    const std::string& device_id,
    mojom::DevicePolicy policy,
    SetDevicePolicyCallback callback) {
  std::move(callback).Run(service_->SetDevicePolicy(device_id, policy));
}

void BraveTransferPageHandler::RemoveDevice(const std::string& device_id,
                                            RemoveDeviceCallback callback) {
  std::move(callback).Run(service_->RemoveDevice(device_id));
}

void BraveTransferPageHandler::SendText(const std::string& device_id,
                                        const std::string& text,
                                        SendTextCallback callback) {
  std::move(callback).Run(service_->SendText(device_id, text));
}

void BraveTransferPageHandler::SendFiles(const std::string& device_id,
                                         std::vector<mojom::FileDataPtr> files,
                                         SendFilesCallback callback) {
  std::move(callback).Run(service_->SendFiles(device_id, std::move(files)));
}

void BraveTransferPageHandler::PickAndSend(const std::string& device_id,
                                           PickAndSendCallback callback) {
  std::move(callback).Run(mojom::SendResult::kNotSupported);
}

void BraveTransferPageHandler::CancelTransfer(const std::string& transfer_id) {
  service_->CancelTransfer(transfer_id);
}

void BraveTransferPageHandler::GetTransfers(GetTransfersCallback callback) {
  std::move(callback).Run(service_->GetTransfers());
}

void BraveTransferPageHandler::GetPendingOffers(
    GetPendingOffersCallback callback) {
  std::move(callback).Run(service_->GetPendingOffers());
}

void BraveTransferPageHandler::RespondToOffer(const std::string& offer_id,
                                              bool accept,
                                              RespondToOfferCallback callback) {
  std::move(callback).Run(service_->RespondToOffer(offer_id, accept));
}

void BraveTransferPageHandler::GetInbox(uint32_t offset,
                                        uint32_t count,
                                        GetInboxCallback callback) {
  service_->GetInbox(offset, count, std::move(callback));
}

void BraveTransferPageHandler::OpenItem(const std::string& item_id,
                                        OpenItemCallback callback) {
  service_->GetInboxEntry(
      item_id, base::BindOnce(&BraveTransferPageHandler::RunItemAction,
                              weak_factory_.GetWeakPtr(), ItemAction::kOpen,
                              std::move(callback)));
}

void BraveTransferPageHandler::ShowItemInFolder(
    const std::string& item_id,
    ShowItemInFolderCallback callback) {
  service_->GetInboxEntry(
      item_id, base::BindOnce(&BraveTransferPageHandler::RunItemAction,
                              weak_factory_.GetWeakPtr(),
                              ItemAction::kShowInFolder, std::move(callback)));
}

void BraveTransferPageHandler::OpenLink(const std::string& item_id,
                                        OpenLinkCallback callback) {
  service_->GetInboxEntry(
      item_id, base::BindOnce(&BraveTransferPageHandler::RunItemAction,
                              weak_factory_.GetWeakPtr(), ItemAction::kOpenLink,
                              std::move(callback)));
}

void BraveTransferPageHandler::ClearHistory(ClearHistoryCallback callback) {
  service_->ClearHistory(std::move(callback));
}

void BraveTransferPageHandler::GetSettings(GetSettingsCallback callback) {
  std::move(callback).Run(service_->GetSettings());
}

void BraveTransferPageHandler::UpdateSettings(mojom::SettingsPtr settings,
                                              UpdateSettingsCallback callback) {
  std::move(callback).Run(service_->UpdateSettings(std::move(settings)));
}

void BraveTransferPageHandler::ChooseSaveDirectory(
    ChooseSaveDirectoryCallback callback) {
  if (select_file_dialog_ || !web_contents_) {
    std::move(callback).Run(false);
    return;
  }
  choose_directory_callback_ = std::move(callback);
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents_));
  mojom::SettingsPtr settings = service_->GetSettings();
  const base::FilePath default_path = base::FilePath::FromUTF8Unsafe(
      settings->save_directory.empty() ? settings->default_save_directory
                                       : settings->save_directory);
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_FOLDER,
      l10n_util::GetStringUTF16(IDS_BRAVE_TRANSFER_CHOOSE_FOLDER_TITLE),
      default_path, /*file_types=*/nullptr, /*file_type_index=*/0,
      /*default_extension=*/base::FilePath::StringType(),
      web_contents_->GetTopLevelNativeWindow());
}

void BraveTransferPageHandler::GetPermissions(GetPermissionsCallback callback) {
  std::move(callback).Run(mojom::Permissions::New());
}

void BraveTransferPageHandler::RequestPermissions(
    RequestPermissionsCallback callback) {
  std::move(callback).Run(mojom::Permissions::New());
}

void BraveTransferPageHandler::OpenSystemSettings() {}

void BraveTransferPageHandler::OnServiceStateChanged() {
  page_->OnServiceStateChanged(service_->GetServiceState());
}

void BraveTransferPageHandler::OnDevicesChanged() {
  page_->OnDevicesChanged(service_->GetDevices());
}

void BraveTransferPageHandler::OnOfferReceived(
    const mojom::IncomingOffer& offer) {
  page_->OnOfferReceived(offer.Clone());
}

void BraveTransferPageHandler::OnOfferRemoved(const std::string& offer_id) {
  page_->OnOfferRemoved(offer_id);
}

void BraveTransferPageHandler::OnTransferUpdated(
    const mojom::Transfer& transfer) {
  page_->OnTransferUpdated(transfer.Clone());
}

void BraveTransferPageHandler::OnInboxChanged() {
  page_->OnInboxChanged();
}

void BraveTransferPageHandler::OnSettingsChanged() {
  page_->OnSettingsChanged(service_->GetSettings());
}

void BraveTransferPageHandler::FileSelected(const ui::SelectedFileInfo& file,
                                            int index) {
  select_file_dialog_.reset();
  service_->SetSaveDirectory(file.path());
  if (choose_directory_callback_) {
    std::move(choose_directory_callback_).Run(true);
  }
}

void BraveTransferPageHandler::FileSelectionCanceled() {
  select_file_dialog_.reset();
  if (choose_directory_callback_) {
    std::move(choose_directory_callback_).Run(false);
  }
}

void BraveTransferPageHandler::RunItemAction(
    ItemAction action,
    base::OnceCallback<void(bool)> callback,
    std::optional<InboxEntry> entry) {
  if (!entry) {
    std::move(callback).Run(false);
    return;
  }
  switch (action) {
    case ItemAction::kOpen:
    case ItemAction::kShowInFolder:
      if (entry->file_path.empty() || entry->file_missing) {
        std::move(callback).Run(false);
        return;
      }
      if (action == ItemAction::kOpen) {
        platform_util::OpenItem(GetProfile(), entry->file_path,
                                platform_util::OPEN_FILE, base::DoNothing());
      } else {
        platform_util::ShowItemInFolder(GetProfile(), entry->file_path);
      }
      std::move(callback).Run(true);
      return;
    case ItemAction::kOpenLink: {
      const GURL url(entry->text);
      if (entry->kind != mojom::ItemKind::kLink || !url.is_valid() ||
          !url.SchemeIsHTTPOrHTTPS()) {
        std::move(callback).Run(false);
        return;
      }
      NavigateParams params(GetProfile(), url, ui::PAGE_TRANSITION_LINK);
      params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
      Navigate(&params);
      std::move(callback).Run(true);
      return;
    }
  }
}

Profile* BraveTransferPageHandler::GetProfile() {
  return Profile::FromBrowserContext(web_contents_->GetBrowserContext());
}

}  // namespace brave_transfer
