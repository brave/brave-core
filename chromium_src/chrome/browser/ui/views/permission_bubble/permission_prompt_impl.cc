/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/browser/widevine/widevine_permission_request.h"
#include "chrome/browser/permissions/permission_request.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/chrome_typography.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/controls/label.h"
#include "ui/views/window/dialog_delegate.h"

namespace {

bool HasWidevinePermissionRequest(
    const std::vector<PermissionRequest*>& requests) {
  // When widevine permission is requested, |requests| only includes Widevine
  // permission because it is not a candidate for grouping.
  if (requests.size() == 1 && requests[0]->GetPermissionRequestType() ==
                                  PermissionRequestType::PERMISSION_WIDEVINE)
    return true;

  return false;
}

void AddWidevineExplanatoryMessageTextIfNeeded(
    views::DialogDelegateView* dialog_delegate,
    const std::vector<PermissionRequest*>& requests) {
  if (!HasWidevinePermissionRequest(requests))
    return;

  auto* widevine_request = static_cast<WidevinePermissionRequest*>(requests[0]);
  views::Label* text =
      new views::Label(widevine_request->GetExplanatoryMessageText(),
                       views::style::CONTEXT_LABEL,
                       STYLE_SECONDARY);
  text->SetMultiLine(true);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();
  const int preferred_dialog_width = provider->GetSnappedDialogWidth(
      dialog_delegate->GetPreferredSize().width());
  // Resize width. Then, it's height deduced.
  text->SizeToFit(preferred_dialog_width - dialog_delegate->margins().width());
  dialog_delegate->AddChildView(text);
}

}  // namespace

#include "../../../../../../../chrome/browser/ui/views/permission_bubble/permission_prompt_impl.cc"  // NOLINT
