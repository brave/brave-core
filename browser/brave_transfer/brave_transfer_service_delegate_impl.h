// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_TRANSFER_BRAVE_TRANSFER_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_TRANSFER_BRAVE_TRANSFER_SERVICE_DELEGATE_IMPL_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_transfer/browser/brave_transfer_service.h"
#include "brave/components/brave_transfer/browser/incoming_file_sink.h"

class Profile;

namespace brave_transfer {

// Saves received files to the folder chosen on brave://transfer, falling back
// to the profile's download folder, and quarantines them like downloads.
std::unique_ptr<IncomingFileSink> CreateDownloadsFileSink(Profile* profile);

// Desktop embedder hooks: the download folder and system notifications that
// open brave://transfer when clicked.
class BraveTransferServiceDelegateImpl : public BraveTransferService::Delegate {
 public:
  explicit BraveTransferServiceDelegateImpl(Profile* profile);
  BraveTransferServiceDelegateImpl(const BraveTransferServiceDelegateImpl&) =
      delete;
  BraveTransferServiceDelegateImpl& operator=(
      const BraveTransferServiceDelegateImpl&) = delete;
  ~BraveTransferServiceDelegateImpl() override;

  // BraveTransferService::Delegate:
  base::FilePath GetDefaultSaveDirectory() override;
  void NotifyOfferPending(const mojom::IncomingOffer& offer) override;
  void NotifyItemsReceived(const std::string& device_name,
                           size_t count) override;
  void NotifyKeyMismatch(const std::string& device_name) override;

 private:
  void ShowNotification(const std::string& id,
                        const std::u16string& title,
                        const std::u16string& message);

  const raw_ptr<Profile> profile_;
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_BRAVE_TRANSFER_BRAVE_TRANSFER_SERVICE_DELEGATE_IMPL_H_
