/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/chrome_speech_recognition_manager_delegate.h"

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

#define CheckRenderFrameType(...) CheckRenderFrameType_ChromiumImpl(__VA_ARGS__)
#include "src/chrome/browser/speech/chrome_speech_recognition_manager_delegate.cc"
#undef CheckRenderFrameType

namespace speech {

// static
void ChromeSpeechRecognitionManagerDelegate::CheckRenderFrameType(
    base::OnceCallback<void(bool ask_user, bool is_allowed)> callback,
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (auto* rph = content::RenderProcessHost::FromID(render_process_id)) {
    if (auto* profile = Profile::FromBrowserContext(rph->GetBrowserContext())) {
      if (profile->IsTor()) {
        // Disable speech recongition in Tor.
        content::GetIOThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(std::move(callback), false, false));
        return;
      }
    }
  }

  return CheckRenderFrameType_ChromiumImpl(std::move(callback),
                                           render_process_id, render_frame_id);
}

}  // namespace speech
