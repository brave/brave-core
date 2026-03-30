/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_HID_CHOOSER_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_HID_CHOOSER_H_

#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/permissions/chooser_controller.h"
#include "content/public/browser/hid_chooser.h"
#include "third_party/blink/public/mojom/hid/hid.mojom-forward.h"

class HidChooserController;

namespace content {
class RenderFrameHost;
}  // namespace content

// Headless HID chooser for Brave Wallet WebUI. Creates a standard
// HidChooserController and subscribes as its View. When
// OnOptionsInitialized fires, auto-selects the first Ledger device
// without showing a picker dialog. Only accepts requestDevice calls with
// exactly `filters: [{ vendorId: 0x2c97 }]`; everything else completes
// with an empty device list.
class BraveWalletHidChooser : public content::HidChooser,
                              public permissions::ChooserController::View {
 public:
  static bool IsBraveWalletMainFrameOrigin(
      content::RenderFrameHost* render_frame_host);

  BraveWalletHidChooser(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
      content::HidChooser::Callback callback);

  BraveWalletHidChooser(const BraveWalletHidChooser&) = delete;
  BraveWalletHidChooser& operator=(const BraveWalletHidChooser&) = delete;

  ~BraveWalletHidChooser() override;

 private:
  // permissions::ChooserController::View:
  void OnOptionsInitialized() override;
  void OnOptionAdded(size_t index) override;
  void OnOptionRemoved(size_t index) override;
  void OnOptionUpdated(size_t index) override;
  void OnAdapterEnabledChanged(bool enabled) override;
  void OnRefreshStateChanged(bool refreshing) override;

  bool is_valid_ledger_request_ = false;
  std::unique_ptr<HidChooserController> controller_;

  base::WeakPtrFactory<BraveWalletHidChooser> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_HID_CHOOSER_H_
