/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ui/base/clipboard/test/privacy_capturing_test_clipboard.h"

#include <memory>
#include <utility>
#include <vector>

namespace brave {

// static
PrivacyCapturingTestClipboard*
PrivacyCapturingTestClipboard::InstallForCurrentThread() {
  // Destroy first because SetClipboardForCurrentThread() DCHECKs when this
  // thread already has a clipboard, which the browser creates during startup.
  ui::Clipboard::DestroyClipboardForCurrentThread();
  auto clipboard = std::make_unique<PrivacyCapturingTestClipboard>();
  auto* clipboard_ptr = clipboard.get();
  ui::Clipboard::SetClipboardForCurrentThread(std::move(clipboard));
  return clipboard_ptr;
}

PrivacyCapturingTestClipboard::PrivacyCapturingTestClipboard() = default;

PrivacyCapturingTestClipboard::~PrivacyCapturingTestClipboard() = default;

void PrivacyCapturingTestClipboard::WritePortableAndPlatformRepresentations(
    ui::ClipboardBuffer buffer,
    const ui::Clipboard::ObjectMap& objects,
    const std::vector<ui::Clipboard::RawData>& raw_objects,
    std::vector<ui::Clipboard::PlatformRepresentation> platform_representations,
    std::unique_ptr<ui::DataTransferEndpoint> data_src,
    uint32_t privacy_types) {
  last_privacy_types_ = privacy_types;
  ui::TestClipboard::WritePortableAndPlatformRepresentations(
      buffer, objects, raw_objects, std::move(platform_representations),
      std::move(data_src), privacy_types);
}

}  // namespace brave
