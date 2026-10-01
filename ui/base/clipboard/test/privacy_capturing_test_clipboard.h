/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BASE_CLIPBOARD_TEST_PRIVACY_CAPTURING_TEST_CLIPBOARD_H_
#define BRAVE_UI_BASE_CLIPBOARD_TEST_PRIVACY_CAPTURING_TEST_CLIPBOARD_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/test/test_clipboard.h"

namespace brave {

// Records the `privacy_types` bitmask that reaches the platform clipboard so
// tests can assert how a write was marked (off-the-record, confidential, ...).
class PrivacyCapturingTestClipboard : public ui::TestClipboard {
 public:
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

// Installs a PrivacyCapturingTestClipboard on the current thread and restores
// the thread's previous clipboard on destruction. Scoping this matters because
// a failed ASSERT_* returns early from the test body, so any manual teardown at
// the end of the body is skipped and the fake would leak into later tests
// sharing the process.
class ScopedPrivacyCapturingTestClipboard {
 public:
  ScopedPrivacyCapturingTestClipboard();
  ScopedPrivacyCapturingTestClipboard(
      const ScopedPrivacyCapturingTestClipboard&) = delete;
  ScopedPrivacyCapturingTestClipboard& operator=(
      const ScopedPrivacyCapturingTestClipboard&) = delete;
  ~ScopedPrivacyCapturingTestClipboard();

  uint32_t last_privacy_types() const {
    return clipboard_->last_privacy_types();
  }

 private:
  std::unique_ptr<ui::Clipboard> previous_clipboard_;
  raw_ptr<PrivacyCapturingTestClipboard> clipboard_ = nullptr;
};

}  // namespace brave

#endif  // BRAVE_UI_BASE_CLIPBOARD_TEST_PRIVACY_CAPTURING_TEST_CLIPBOARD_H_
