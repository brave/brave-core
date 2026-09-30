/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BASE_CLIPBOARD_TEST_PRIVACY_CAPTURING_TEST_CLIPBOARD_H_
#define BRAVE_UI_BASE_CLIPBOARD_TEST_PRIVACY_CAPTURING_TEST_CLIPBOARD_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/test/test_clipboard.h"

namespace brave {

// Records the `privacy_types` bitmask that reaches the platform clipboard so
// tests can assert how a write was marked (off-the-record, confidential, ...).
class PrivacyCapturingTestClipboard : public ui::TestClipboard {
 public:
  // Replaces the current thread's clipboard with a recording one. The returned
  // instance stays owned by the thread's clipboard registry; release it with
  // ui::Clipboard::DestroyClipboardForCurrentThread().
  static PrivacyCapturingTestClipboard* InstallForCurrentThread();

  PrivacyCapturingTestClipboard();
  ~PrivacyCapturingTestClipboard() override;

  uint32_t last_privacy_types() const { return last_privacy_types_; }

  // ui::TestClipboard:
  void WritePortableAndPlatformRepresentations(
      ui::ClipboardBuffer buffer,
      const ui::Clipboard::ObjectMap& objects,
      const std::vector<ui::Clipboard::RawData>& raw_objects,
      std::vector<ui::Clipboard::PlatformRepresentation>
          platform_representations,
      std::unique_ptr<ui::DataTransferEndpoint> data_src,
      uint32_t privacy_types) override;

 private:
  uint32_t last_privacy_types_ = ui::Clipboard::kNone;
};

}  // namespace brave

#endif  // BRAVE_UI_BASE_CLIPBOARD_TEST_PRIVACY_CAPTURING_TEST_CLIPBOARD_H_
