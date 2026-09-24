/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/clipboard_host_impl.h"

#include <optional>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "url/gurl.h"

#include <content/browser/renderer_host/clipboard_host_impl.cc>

namespace content {

namespace {

void FrameClipboardContext::MaybeSanitizeClipboardText(
    bool& sanitize_on_next_write_text,
    std::u16string& text) {
  if (!std::exchange(sanitize_on_next_write_text, false) || text.size() < 6 ||
      text.size() > 512u) {
    return;
  }

  const GURL url(text);
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  if (std::optional<GURL> sanitized_url =
          GetContentClient()->browser()->SanitizeURL(
              render_frame_host_->GetMainFrame(), url)) {
    text = base::UTF8ToUTF16(sanitized_url->spec());
  }
}

}  // namespace

void ClipboardHostImpl::Context::MaybeSanitizeClipboardText(
    bool& sanitize_on_next_write_text,
    std::u16string& text) {
  sanitize_on_next_write_text = false;
}

void ClipboardHostImpl::SanitizeOnNextWriteText() {
  sanitize_on_next_write_text_ = true;
}

}  // namespace content
