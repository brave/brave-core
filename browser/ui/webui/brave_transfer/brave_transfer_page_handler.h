// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_transfer/browser/brave_transfer_service.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/shell_dialogs/select_file_dialog.h"

class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace brave_transfer {

// Desktop host for brave://transfer, backed by BraveTransferService.
class BraveTransferPageHandler : public mojom::PageHandler,
                                 public BraveTransferService::Observer,
                                 public ui::SelectFileDialog::Listener {
 public:
  BraveTransferPageHandler(mojo::PendingReceiver<mojom::PageHandler> receiver,
                           mojo::PendingRemote<mojom::Page> page,
                           BraveTransferService* service,
                           content::WebContents* web_contents);
  BraveTransferPageHandler(const BraveTransferPageHandler&) = delete;
  BraveTransferPageHandler& operator=(const BraveTransferPageHandler&) = delete;
  ~BraveTransferPageHandler() override;

  // mojom::PageHandler:
  void GetServiceState(GetServiceStateCallback callback) override;
  void OpenSyncSettings() override;
  void GetDevices(GetDevicesCallback callback) override;
  void SetDevicePolicy(const std::string& device_id,
                       mojom::DevicePolicy policy,
                       SetDevicePolicyCallback callback) override;
  void RemoveDevice(const std::string& device_id,
                    RemoveDeviceCallback callback) override;
  void SendText(const std::string& device_id,
                const std::string& text,
                SendTextCallback callback) override;
  void SendFiles(const std::string& device_id,
                 std::vector<mojom::FileDataPtr> files,
                 SendFilesCallback callback) override;
  void PickAndSend(const std::string& device_id,
                   PickAndSendCallback callback) override;
  void CancelTransfer(const std::string& transfer_id) override;
  void GetTransfers(GetTransfersCallback callback) override;
  void GetPendingOffers(GetPendingOffersCallback callback) override;
  void RespondToOffer(const std::string& offer_id,
                      bool accept,
                      RespondToOfferCallback callback) override;
  void GetInbox(uint32_t offset,
                uint32_t count,
                GetInboxCallback callback) override;
  void OpenItem(const std::string& item_id, OpenItemCallback callback) override;
  void ShowItemInFolder(const std::string& item_id,
                        ShowItemInFolderCallback callback) override;
  void OpenLink(const std::string& item_id, OpenLinkCallback callback) override;
  void ClearHistory(ClearHistoryCallback callback) override;
  void GetSettings(GetSettingsCallback callback) override;
  void UpdateSettings(mojom::SettingsPtr settings,
                      UpdateSettingsCallback callback) override;
  void ChooseSaveDirectory(ChooseSaveDirectoryCallback callback) override;
  void GetPermissions(GetPermissionsCallback callback) override;
  void RequestPermissions(RequestPermissionsCallback callback) override;
  void OpenSystemSettings() override;

 private:
  enum class ItemAction { kOpen, kShowInFolder, kOpenLink };

  // BraveTransferService::Observer:
  void OnServiceStateChanged() override;
  void OnDevicesChanged() override;
  void OnOfferReceived(const mojom::IncomingOffer& offer) override;
  void OnOfferRemoved(const std::string& offer_id) override;
  void OnTransferUpdated(const mojom::Transfer& transfer) override;
  void OnInboxChanged() override;
  void OnSettingsChanged() override;

  // ui::SelectFileDialog::Listener:
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void FileSelectionCanceled() override;

  void RunItemAction(ItemAction action,
                     base::OnceCallback<void(bool)> callback,
                     std::optional<InboxEntry> entry);
  Profile* GetProfile();

  mojo::Receiver<mojom::PageHandler> receiver_;
  mojo::Remote<mojom::Page> page_;
  raw_ptr<BraveTransferService> service_;
  raw_ptr<content::WebContents> web_contents_;
  base::ScopedObservation<BraveTransferService, BraveTransferService::Observer>
      service_observation_{this};
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  ChooseSaveDirectoryCallback choose_directory_callback_;
  base::WeakPtrFactory<BraveTransferPageHandler> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_H_
