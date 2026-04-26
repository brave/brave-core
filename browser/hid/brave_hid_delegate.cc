/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/hid/brave_hid_delegate.h"

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "content/public/browser/render_frame_host.h"
#include "third_party/blink/public/mojom/hid/hid.mojom.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/browser/brave_wallet/brave_wallet_hid_chooser.h"
#endif

BraveHidDelegate::BraveHidDelegate() = default;
BraveHidDelegate::~BraveHidDelegate() = default;

std::unique_ptr<content::HidChooser> BraveHidDelegate::RunChooser(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
    content::HidChooser::Callback callback) {
  DCHECK(render_frame_host);

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  if (BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(render_frame_host)) {
    return std::make_unique<BraveWalletHidChooser>(
        render_frame_host, std::move(filters), std::move(exclusion_filters),
        std::move(callback));
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

  return ChromeHidDelegate::RunChooser(render_frame_host, std::move(filters),
                                       std::move(exclusion_filters),
                                       std::move(callback));
}
