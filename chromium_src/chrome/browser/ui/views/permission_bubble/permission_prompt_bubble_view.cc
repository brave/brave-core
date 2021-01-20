/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "components/permissions/permission_request.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/window/dialog_delegate.h"

#if BUILDFLAG(ENABLE_WIDEVINE)
#include "brave/browser/widevine/widevine_permission_request.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/permissions/request_type.h"
#include "ui/gfx/text_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/style/typography.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_WIDEVINE)
class DontAskAgainCheckbox : public views::Checkbox {
 public:
  explicit DontAskAgainCheckbox(WidevinePermissionRequest* request);

 private:
  void ButtonPressed();

  WidevinePermissionRequest* request_;

  DISALLOW_COPY_AND_ASSIGN(DontAskAgainCheckbox);
};

DontAskAgainCheckbox::DontAskAgainCheckbox(WidevinePermissionRequest* request)
    : views::Checkbox(
          l10n_util::GetStringUTF16(IDS_WIDEVINE_DONT_ASK_AGAIN_CHECKBOX),
          base::BindRepeating(&DontAskAgainCheckbox::ButtonPressed,
                              base::Unretained(this))),
      request_(request) {}

void DontAskAgainCheckbox::ButtonPressed() {
  request_->set_dont_ask_widevine_install(GetChecked());
}

bool HasWidevinePermissionRequest(
    const std::vector<permissions::PermissionRequest*>& requests) {
  // When widevine permission is requested, |requests| only includes Widevine
  // permission because it is not a candidate for grouping.
  if (requests.size() == 1 &&
      requests[0]->GetRequestType() == permissions::RequestType::kWidevine)
    return true;

  return false;
}

void AddAdditionalWidevineViewControlsIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<permissions::PermissionRequest*>& requests) {
  if (!HasWidevinePermissionRequest(requests))
    return;

  auto* widevine_request = static_cast<WidevinePermissionRequest*>(requests[0]);
  views::Label* text = new views::Label(
      widevine_request->GetExplanatoryMessageText(),
      views::style::CONTEXT_LABEL, views::style::STYLE_SECONDARY);
  text->SetMultiLine(true);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const int preferred_dialog_width = provider->GetSnappedDialogWidth(
      dialog_delegate_view->GetPreferredSize().width());
  // Resize width. Then, it's height deduced.
  text->SizeToFit(preferred_dialog_width -
                  dialog_delegate_view->margins().width());
  dialog_delegate_view->AddChildView(text);
  dialog_delegate_view->AddChildView(
      new DontAskAgainCheckbox(widevine_request));
}
#else
void AddAdditionalWidevineViewControlsIfNeeded(
    views::BubbleDialogDelegateView* dialog_delegate_view,
    const std::vector<permissions::PermissionRequest*>& requests) {}
#endif
}  // namespace

#define BRAVE_PERMISSION_PROMPT_BUBBLE_VIEW \
  AddAdditionalWidevineViewControlsIfNeeded(this, delegate_->Requests());

#include "../../../../../../../chrome/browser/ui/views/permission_bubble/permission_prompt_bubble_view.cc"
#undef BRAVE_PERMISSION_PROMPT_BUBBLE_VIEW
