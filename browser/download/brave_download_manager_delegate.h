/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_MANAGER_DELEGATE_H_
#define BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_MANAGER_DELEGATE_H_

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"
#include "chrome/browser/download/chrome_download_manager_delegate.h"

class Profile;

class BraveDownloadManagerDelegate : public ChromeDownloadManagerDelegate {
 public:
  // Similar to SafeBrowsingState in the upstream to prevent repeated calls to
  // iptc stripping and block the download.
  struct IptcStrippingState : public DownloadCompletionBlocker {
    constexpr static char kUserDataKey[] =
        "brave.image_metadata_strip_data_key";

    IptcStrippingState() = default;
    ~IptcStrippingState() override = default;

    IptcStrippingState(const IptcStrippingState&) = delete;
    IptcStrippingState& operator=(const IptcStrippingState&) = delete;

    // A flag to ensure we perform the stripping only once.
    bool stripping_started = false;
  };

  explicit BraveDownloadManagerDelegate(Profile* profile);
  BraveDownloadManagerDelegate(const BraveDownloadManagerDelegate&) = delete;
  BraveDownloadManagerDelegate& operator=(const BraveDownloadManagerDelegate&) =
      delete;
  ~BraveDownloadManagerDelegate() override;

 protected:
  // Marks the download as completed once the iptc metadata is stripped. Virtual
  // for testing purposes.
  virtual void OnImageMetadataStripped(
      uint32_t download_id,
      image_metadata_stripper::StrippingResultCode result);

 private:
  // ChromeDownloadManagerDelegate override.
  bool IsDownloadReadyForCompletion(
      download::DownloadItem* item,
      base::OnceClosure internal_complete_callback) override;

  base::WeakPtrFactory<BraveDownloadManagerDelegate> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_MANAGER_DELEGATE_H_
