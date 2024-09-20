/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/clipboard_host_impl.h"

#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "url/gurl.h"

namespace {

std::u16string Sanitize(content::ContentBrowserClient* client,
                        content::RenderFrameHost* render_frame_host,
                        std::u16string data) {
  if (!client || !render_frame_host ||
      !render_frame_host->GetBrowserContext() || data.size() < 6 ||
      data.size() > 512u) {
    return data;
  }

  const GURL url(data);
  if (url.is_valid() && !url.is_empty() && url.SchemeIsHTTPOrHTTPS()) {
    std::optional<GURL> sanitized_url =
        client->SanitizeURL(render_frame_host, url);
    if (!sanitized_url) {
      return data;
    }
    return base::UTF8ToUTF16(sanitized_url->spec());
  }
  return data;
}

}  // namespace

#define BRAVE_CLIPBOARD_HOST_IMPL_SANITIZE                                  \
  if (sanitize_on_next_write_text_) {                                       \
    data.text =                                                             \
        Sanitize(GetContentClient()->browser(),                             \
                 render_frame_host().GetMainFrame(), std::move(data.text)); \
  }                                                                         \
  sanitize_on_next_write_text_ = false;

#include "src/content/browser/renderer_host/clipboard_host_impl.cc"

#undef BRAVE_CLIPBOARD_HOST_IMPL_SANITIZE

namespace content {

void ClipboardHostImpl::SanitizeOnNextWriteText() {
  sanitize_on_next_write_text_ = true;
}

}  // namespace content
