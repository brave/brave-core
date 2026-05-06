/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_HID_BRAVE_HID_DELEGATE_H_
#define BRAVE_BROWSER_HID_BRAVE_HID_DELEGATE_H_

#include <memory>
#include <vector>

#include "chrome/browser/hid/chrome_hid_delegate.h"
#include "content/public/browser/hid_chooser.h"
#include "content/public/browser/hid_delegate.h"
#include "third_party/blink/public/mojom/hid/hid.mojom-forward.h"

namespace content {
class RenderFrameHost;
}

// Extends `ChromeHidDelegate` so there is custom behavior for HID requests from
// wallet's ledger webui pages.
class BraveHidDelegate : public ChromeHidDelegate {
 public:
  BraveHidDelegate();
  BraveHidDelegate(const BraveHidDelegate&) = delete;
  BraveHidDelegate& operator=(const BraveHidDelegate&) = delete;
  ~BraveHidDelegate() override;

  std::unique_ptr<content::HidChooser> RunChooser(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
      content::HidChooser::Callback callback) override;
};

#endif  // BRAVE_BROWSER_HID_BRAVE_HID_DELEGATE_H_
