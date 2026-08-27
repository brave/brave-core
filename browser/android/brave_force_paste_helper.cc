/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/functional/bind.h"
#include "chrome/android/chrome_jni_headers/BraveForcePasteHelper_jni.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_buffer.h"
#include "ui/base/clipboard/data_transfer_endpoint.h"

namespace brave_shields {

static void JNI_BraveForcePasteHelper_ForcePaste(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  if (!web_contents) {
    return;
  }

  content::RenderFrameHost* frame = web_contents->GetFocusedFrame();
  if (!frame) {
    return;
  }

  std::optional<ui::DataTransferEndpoint> data = ui::DataTransferEndpoint(
      frame->GetMainFrame()->GetLastCommittedURL(),
      ui::DataTransferEndpointOptions{
          .notify_if_restricted = true,
          .off_the_record = frame->GetBrowserContext()->IsOffTheRecord()});

  ui::Clipboard::GetForCurrentThread()->ReadText(
      ui::ClipboardBuffer::kCopyPaste, data,
      base::BindOnce(
          [](base::WeakPtr<content::WebContents> web_contents,
             std::u16string result) {
            if (!web_contents || result.empty()) {
              return;
            }

            web_contents->Replace(result);
          },
          web_contents->GetWeakPtr()));
}

}  // namespace brave_shields

DEFINE_JNI(BraveForcePasteHelper)
