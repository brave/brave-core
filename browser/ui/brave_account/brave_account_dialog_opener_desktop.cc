/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_account/brave_account_dialog_opener.h"

#include <memory>

#include "base/check_deref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_account/dialog_mode_holder.h"
#include "brave/components/brave_account/brave_account_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "net/base/url_util.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"

namespace brave_account {

namespace {

constexpr float kDialogBorderRadius = 16;
constexpr int kDialogWidth = 500;
constexpr gfx::Size kDialogMinSize(kDialogWidth, 300);
constexpr gfx::Size kDialogMaxSize(kDialogWidth, 800);

// Tracks whether a Brave Account dialog is open for a WebContents.
// This prevents multiple dialogs from being created via rapid clicks.
class BraveAccountDialogTracker
    : public content::WebContentsUserData<BraveAccountDialogTracker> {
 public:
  ~BraveAccountDialogTracker() override = default;

 private:
  friend class content::WebContentsUserData<BraveAccountDialogTracker>;

  explicit BraveAccountDialogTracker(content::WebContents* web_contents)
      : content::WebContentsUserData<BraveAccountDialogTracker>(*web_contents) {
  }

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveAccountDialogTracker);

class BraveAccountDialogDelegate : public ui::WebDialogDelegate {
 public:
  BraveAccountDialogDelegate(content::WebContents* web_contents,
                             const std::string& initiating_service_name)
      : web_contents_(CHECK_DEREF(web_contents).GetWeakPtr()) {
    BraveAccountDialogTracker::CreateForWebContents(web_contents);

    set_delete_on_close(false);
    const GURL url(kBraveAccountURL);
    set_dialog_content_url(
        initiating_service_name.empty()
            ? url
            : net::AppendQueryParameter(url, kInitiatingServiceNameQueryParam,
                                        initiating_service_name));
    set_show_dialog_title(false);
  }

  ~BraveAccountDialogDelegate() override {
    if (web_contents_) {
      web_contents_->RemoveUserData(BraveAccountDialogTracker::UserDataKey());
    }
  }

 private:
  base::WeakPtr<content::WebContents> web_contents_;
};

}  // namespace

void OpenBraveAccountDialog(content::WebContents& web_contents,
                            const std::string& initiating_service_name,
                            mojom::DialogMode dialog_mode) {
  if (BraveAccountDialogTracker::FromWebContents(&web_contents)) {
    return;
  }

  auto& delegate = CHECK_DEREF(ShowConstrainedWebDialogWithAutoResize(
      Profile::FromBrowserContext(web_contents.GetBrowserContext()),
      std::make_unique<BraveAccountDialogDelegate>(&web_contents,
                                                   initiating_service_name),
      &web_contents, kDialogMinSize, kDialogMaxSize));

  // On the dialog's own `WebContents` - the one that hosts brave://account,
  // not the opener's.
  DialogModeHolder::SetDialogMode(CHECK_DEREF(delegate.GetWebContents()),
                                  dialog_mode);

  auto* widget =
      views::Widget::GetWidgetForNativeWindow(delegate.GetNativeDialog());
  if (!widget) {
    return;
  }

  if (auto* layer = widget->GetLayer()) {
    layer->SetRoundedCornerRadius(gfx::RoundedCornersF(kDialogBorderRadius));
  }
}

}  // namespace brave_account
