// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_transfer/brave_transfer_page_handler.h"

#import <Foundation/Foundation.h>

#include <memory>
#include <utility>

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.objc+private.h"
#include "brave/components/brave_transfer/ios/browser/transfer_page_handler_bridge.h"
#include "brave/ios/browser/ui/webui/brave_transfer/brave_transfer_page_handler_bridge_holder.h"
#include "ios/web/public/web_state.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace brave_transfer {

namespace {

// Mojo replies must be run even when the Swift side drops a completion (or no
// bridge is attached), so each reply is wrapped to run with `defaults` when
// the block holding it is released without calling it. The bridge contract
// keeps completions, and so the release, on the main thread.
template <typename Callback, typename... Defaults>
std::shared_ptr<Callback> ShareReply(Callback callback,
                                     Defaults&&... defaults) {
  return std::make_shared<Callback>(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), std::forward<Defaults>(defaults)...));
}

template <typename Callback, typename... Args>
void RunReply(const std::shared_ptr<Callback>& reply, Args&&... args) {
  if (*reply) {
    std::move(*reply).Run(std::forward<Args>(args)...);
  }
}

mojom::ServiceStatePtr ToMojo(BraveTransferServiceState* state) {
  return state ? [state cppObjPtr] : mojom::ServiceState::New();
}

mojom::SettingsPtr ToMojo(BraveTransferSettings* settings) {
  return settings ? [settings cppObjPtr] : mojom::Settings::New();
}

mojom::PermissionsPtr ToMojo(BraveTransferPermissions* permissions) {
  return permissions ? [permissions cppObjPtr] : mojom::Permissions::New();
}

std::vector<mojom::DevicePtr> ToMojo(NSArray<BraveTransferDevice*>* devices) {
  std::vector<mojom::DevicePtr> result;
  for (BraveTransferDevice* device in devices) {
    result.push_back([device cppObjPtr]);
  }
  return result;
}

std::vector<mojom::TransferPtr> ToMojo(
    NSArray<BraveTransferTransfer*>* transfers) {
  std::vector<mojom::TransferPtr> result;
  for (BraveTransferTransfer* transfer in transfers) {
    result.push_back([transfer cppObjPtr]);
  }
  return result;
}

std::vector<mojom::IncomingOfferPtr> ToMojo(
    NSArray<BraveTransferIncomingOffer*>* offers) {
  std::vector<mojom::IncomingOfferPtr> result;
  for (BraveTransferIncomingOffer* offer in offers) {
    result.push_back([offer cppObjPtr]);
  }
  return result;
}

std::vector<mojom::InboxItemPtr> ToMojo(
    NSArray<BraveTransferInboxItem*>* items) {
  std::vector<mojom::InboxItemPtr> result;
  for (BraveTransferInboxItem* item in items) {
    result.push_back([item cppObjPtr]);
  }
  return result;
}

}  // namespace

BraveTransferPageHandler::BraveTransferPageHandler(
    web::WebState* web_state,
    mojo::PendingReceiver<mojom::PageHandler> receiver,
    mojo::PendingRemote<mojom::Page> page)
    : web_state_(web_state),
      receiver_(this, std::move(receiver)),
      page_([[BraveTransferPageMojoImpl alloc] initWithPage:std::move(page)]) {
  [GetBridge() pageDidConnect:page_];
}

BraveTransferPageHandler::~BraveTransferPageHandler() {
  [GetBridge() pageDidDisconnect:page_];
}

id<TransferPageHandlerBridge> BraveTransferPageHandler::GetBridge() {
  auto* holder = PageHandlerBridgeHolder::FromWebState(web_state_);
  return holder ? holder->bridge() : nil;
}

void BraveTransferPageHandler::GetServiceState(
    GetServiceStateCallback callback) {
  auto reply = ShareReply(std::move(callback), mojom::ServiceState::New());
  [GetBridge() serviceState:^(BraveTransferServiceState* state) {
    RunReply(reply, ToMojo(state));
  }];
}

void BraveTransferPageHandler::OpenSyncSettings() {
  [GetBridge() openSyncSettings];
}

void BraveTransferPageHandler::GetDevices(GetDevicesCallback callback) {
  auto reply =
      ShareReply(std::move(callback), std::vector<mojom::DevicePtr>());
  [GetBridge() devices:^(NSArray<BraveTransferDevice*>* devices) {
    RunReply(reply, ToMojo(devices));
  }];
}

void BraveTransferPageHandler::SetDevicePolicy(
    const std::string& device_id,
    mojom::DevicePolicy policy,
    SetDevicePolicyCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() setDevicePolicy:base::SysUTF8ToNSString(device_id)
                        policy:static_cast<BraveTransferDevicePolicy>(policy)
                    completion:^(bool success) {
                      RunReply(reply, success);
                    }];
}

void BraveTransferPageHandler::RemoveDevice(const std::string& device_id,
                                            RemoveDeviceCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() removeDevice:base::SysUTF8ToNSString(device_id)
                 completion:^(bool success) {
                   RunReply(reply, success);
                 }];
}

void BraveTransferPageHandler::SendText(const std::string& device_id,
                                        const std::string& text,
                                        SendTextCallback callback) {
  auto reply = ShareReply(std::move(callback), mojom::SendResult::kFailed);
  [GetBridge() sendText:base::SysUTF8ToNSString(device_id)
                   text:base::SysUTF8ToNSString(text)
             completion:^(BraveTransferSendResult result) {
               RunReply(reply, static_cast<mojom::SendResult>(result));
             }];
}

void BraveTransferPageHandler::SendFiles(const std::string& device_id,
                                         std::vector<mojom::FileDataPtr> files,
                                         SendFilesCallback callback) {
  // The iOS page sends files through PickAndSend, so the file bytes are never
  // copied across the bridge.
  std::move(callback).Run(mojom::SendResult::kNotSupported);
}

void BraveTransferPageHandler::PickAndSend(const std::string& device_id,
                                           PickAndSendCallback callback) {
  auto reply = ShareReply(std::move(callback), mojom::SendResult::kFailed);
  [GetBridge() pickAndSend:base::SysUTF8ToNSString(device_id)
                completion:^(BraveTransferSendResult result) {
                  RunReply(reply, static_cast<mojom::SendResult>(result));
                }];
}

void BraveTransferPageHandler::CancelTransfer(const std::string& transfer_id) {
  [GetBridge() cancelTransfer:base::SysUTF8ToNSString(transfer_id)];
}

void BraveTransferPageHandler::GetTransfers(GetTransfersCallback callback) {
  auto reply =
      ShareReply(std::move(callback), std::vector<mojom::TransferPtr>());
  [GetBridge() transfers:^(NSArray<BraveTransferTransfer*>* transfers) {
    RunReply(reply, ToMojo(transfers));
  }];
}

void BraveTransferPageHandler::GetPendingOffers(
    GetPendingOffersCallback callback) {
  auto reply =
      ShareReply(std::move(callback), std::vector<mojom::IncomingOfferPtr>());
  [GetBridge() pendingOffers:^(NSArray<BraveTransferIncomingOffer*>* offers) {
    RunReply(reply, ToMojo(offers));
  }];
}

void BraveTransferPageHandler::RespondToOffer(const std::string& offer_id,
                                              bool accept,
                                              RespondToOfferCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() respondToOffer:base::SysUTF8ToNSString(offer_id)
                       accept:accept
                   completion:^(bool success) {
                     RunReply(reply, success);
                   }];
}

void BraveTransferPageHandler::GetInbox(uint32_t offset,
                                        uint32_t count,
                                        GetInboxCallback callback) {
  auto reply = ShareReply(std::move(callback),
                          std::vector<mojom::InboxItemPtr>(), false);
  [GetBridge() inbox:offset
               count:count
          completion:^(NSArray<BraveTransferInboxItem*>* items, bool has_more) {
            RunReply(reply, ToMojo(items), has_more);
          }];
}

void BraveTransferPageHandler::OpenItem(const std::string& item_id,
                                        OpenItemCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() openItem:base::SysUTF8ToNSString(item_id)
             completion:^(bool success) {
               RunReply(reply, success);
             }];
}

void BraveTransferPageHandler::ShowItemInFolder(
    const std::string& item_id,
    ShowItemInFolderCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() showItemInFolder:base::SysUTF8ToNSString(item_id)
                     completion:^(bool success) {
                       RunReply(reply, success);
                     }];
}

void BraveTransferPageHandler::OpenLink(const std::string& item_id,
                                        OpenLinkCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() openLink:base::SysUTF8ToNSString(item_id)
             completion:^(bool success) {
               RunReply(reply, success);
             }];
}

void BraveTransferPageHandler::ClearHistory(ClearHistoryCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  [GetBridge() clearHistory:^(bool success) {
    RunReply(reply, success);
  }];
}

void BraveTransferPageHandler::GetSettings(GetSettingsCallback callback) {
  auto reply = ShareReply(std::move(callback), mojom::Settings::New());
  [GetBridge() settings:^(BraveTransferSettings* settings) {
    RunReply(reply, ToMojo(settings));
  }];
}

void BraveTransferPageHandler::UpdateSettings(mojom::SettingsPtr settings,
                                              UpdateSettingsCallback callback) {
  auto reply = ShareReply(std::move(callback), false);
  BraveTransferSettings* objc_settings =
      [[BraveTransferSettings alloc] initWithSettings:*settings];
  [GetBridge() updateSettings:objc_settings
                   completion:^(bool success) {
                     RunReply(reply, success);
                   }];
}

void BraveTransferPageHandler::ChooseSaveDirectory(
    ChooseSaveDirectoryCallback callback) {
  // iOS saves files to Files and images to Photos; there is no folder choice.
  std::move(callback).Run(false);
}

void BraveTransferPageHandler::GetPermissions(GetPermissionsCallback callback) {
  auto reply = ShareReply(std::move(callback), mojom::Permissions::New());
  [GetBridge() permissions:^(BraveTransferPermissions* permissions) {
    RunReply(reply, ToMojo(permissions));
  }];
}

void BraveTransferPageHandler::RequestPermissions(
    RequestPermissionsCallback callback) {
  auto reply = ShareReply(std::move(callback), mojom::Permissions::New());
  [GetBridge() requestPermissions:^(BraveTransferPermissions* permissions) {
    RunReply(reply, ToMojo(permissions));
  }];
}

void BraveTransferPageHandler::OpenSystemSettings() {
  [GetBridge() openSystemSettings];
}

}  // namespace brave_transfer
