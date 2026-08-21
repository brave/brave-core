/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/scoped_java_ref.h"
#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"
#include "chrome/android/chrome_jni_headers/SponsoredRichMediaWebView_jni.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "third_party/jni_zero/jni_zero.h"
#include "ui/gfx/geometry/rect_f.h"

static void JNI_SponsoredRichMediaWebView_SetSafeArea(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& jweb_contents,
    float x,
    float y,
    float width,
    float height) {
  content::WebContents* const web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  if (!web_contents) {
    return;
  }

  content::WebUI* const web_ui = web_contents->GetWebUI();
  if (!web_ui) {
    return;
  }

  NewTabTakeoverUI* const new_tab_takeover_ui =
      web_ui->GetController()->GetAs<NewTabTakeoverUI>();
  if (!new_tab_takeover_ui) {
    return;
  }

  new_tab_takeover_ui->SetSafeArea(gfx::RectF(x, y, width, height));
}

DEFINE_JNI(SponsoredRichMediaWebView)
