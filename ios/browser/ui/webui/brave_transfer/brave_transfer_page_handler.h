// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

@protocol BraveTransferPage;
@protocol TransferPageHandlerBridge;

namespace web {
class WebState;
}  // namespace web

namespace brave_transfer {

// iOS host for brave://transfer. The engine is Swift, so every call is
// forwarded to the tab's TransferPageHandlerBridge (see
// PageHandlerBridgeHolder). When no bridge is attached, calls with a reply
// are answered with default values.
class BraveTransferPageHandler : public mojom::PageHandler {
 public:
  BraveTransferPageHandler(web::WebState* web_state,
                           mojo::PendingReceiver<mojom::PageHandler> receiver,
                           mojo::PendingRemote<mojom::Page> page);
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
  id<TransferPageHandlerBridge> GetBridge();

  raw_ptr<web::WebState> web_state_;
  mojo::Receiver<mojom::PageHandler> receiver_;
  // Wraps the mojom::Page remote. Handed to the bridge, which may keep it
  // after this handler is gone; calls on it are then dropped.
  __strong id<BraveTransferPage> page_;
};

}  // namespace brave_transfer

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_H_
