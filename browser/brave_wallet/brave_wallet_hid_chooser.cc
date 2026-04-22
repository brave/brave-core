/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_hid_chooser.h"

#include <utility>

#include "base/task/sequenced_task_runner.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/hid/hid_chooser_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/url_constants.h"

namespace {

// https://github.com/LedgerHQ/ledger-live/blob/641a32b87cf83a38fb93dc65322ad4e2f6d3e8b5/libs/ledgerjs/packages/devices/src/index.ts#L161
constexpr uint16_t kLedgerVendorId = 0x2c97;

// Matches `filters: [{ vendorId: LEDGER_VENDOR_ID }]` — vendor only, no
// product id, no usage filter, no exclusion filters, exactly one filter.
// We expect only this request params from Ledger's hw-transport-webhid package:
// https://github.com/LedgerHQ/ledger-live/blob/641a32b87cf83a38fb93dc65322ad4e2f6d3e8b5/libs/ledgerjs/packages/hw-transport-webhid/src/TransportWebHID.ts#L29-L35
bool IsExactlyLedgerVendorOnlyRequest(
    const std::vector<blink::mojom::HidDeviceFilterPtr>& filters,
    const std::vector<blink::mojom::HidDeviceFilterPtr>& exclusion_filters) {
  if (!exclusion_filters.empty() || filters.size() != 1u) {
    return false;
  }

  return filters[0] ==
         blink::mojom::HidDeviceFilter::New(
             blink::mojom::DeviceIdFilter::NewVendor(kLedgerVendorId), nullptr);
}

bool IsLedgerSubframe(content::RenderFrameHost* rfh) {
  // We expect hid request only from ledger subframe.
  if (!rfh || !rfh->GetParent()) {
    return false;
  }
  auto origin = rfh->GetLastCommittedOrigin();
  return origin.scheme() == content::kChromeUIUntrustedScheme &&
         (origin.host() == kUntrustedLedgerHost);
}

}  // namespace

// static
bool BraveWalletHidChooser::IsBraveWalletMainFrameOrigin(
    content::RenderFrameHost* rfh) {
  if (!rfh) {
    return false;
  }

  const auto& origin = rfh->GetMainFrame()->GetLastCommittedOrigin();
  return origin.scheme() == content::kChromeUIScheme &&
         (origin.host() == kWalletPageHost ||
          origin.host() == kWalletPanelHost);
}

BraveWalletHidChooser::BraveWalletHidChooser(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
    content::HidChooser::Callback callback) {
  CHECK(BraveWalletHidChooser::IsBraveWalletMainFrameOrigin(render_frame_host));

  if (!IsLedgerSubframe(render_frame_host) ||
      !IsExactlyLedgerVendorOnlyRequest(filters, exclusion_filters)) {
    // Request comes from non ledger subframe of wallet mainframe, or has
    // unexpected filters. Respond with empty device list.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       std::vector<device::mojom::HidDeviceInfoPtr>()));
    return;
  }

  controller_ = std::make_unique<HidChooserController>(
      render_frame_host, std::move(filters), std::move(exclusion_filters),
      std::move(callback));

  controller_->set_view(this);
}

BraveWalletHidChooser::~BraveWalletHidChooser() = default;

void BraveWalletHidChooser::OnOptionsInitialized() {
  if (controller_->NumOptions() == 0) {
    controller_->Cancel();
  } else {
    controller_->Select({0});
  }

  // Reset controller to run callback and unsubscribe to device events as
  // we need only initial set of devices.
  controller_.reset();
}

void BraveWalletHidChooser::OnOptionAdded(size_t index) {}
void BraveWalletHidChooser::OnOptionRemoved(size_t index) {}
void BraveWalletHidChooser::OnOptionUpdated(size_t index) {}
void BraveWalletHidChooser::OnAdapterEnabledChanged(bool enabled) {}
void BraveWalletHidChooser::OnRefreshStateChanged(bool refreshing) {}
