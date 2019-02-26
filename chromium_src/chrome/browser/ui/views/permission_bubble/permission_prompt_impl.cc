/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/browser/widevine/widevine_permission_request.h"
#include "chrome/browser/permissions/permission_request.h"
#include "ui/gfx/text_constants.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"

namespace {

bool HasWidevinePermissionRequest(
    const std::vector<PermissionRequest*>& requests) {
  // When widevine permission is requested, |requests| only includes Widevine
  // permission because it is not a candidate for grouping.
  if (requests.size() == 1 &&
      requests[0]->GetPermissionRequestType() ==
          PermissionRequestType::PERMISSION_WIDEVINE)
    return true;

  return false;
}

void AddWidevineExplanatoryMessageTextIfNeeded(
    views::View* container,
    int preferred_message_width,
    const std::vector<PermissionRequest*>& requests) {
  if (!HasWidevinePermissionRequest(requests))
    return;

  auto* widevine_request =
      static_cast<WidevinePermissionRequest*>(requests[0]);
  views::Label* text =
      new views::Label(widevine_request->GetExplanatoryMessageText());
  text->SetMultiLine(true);
  text->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  text->SizeToFit(preferred_message_width);
  text->SetBorder(views::CreateEmptyBorder(gfx::Insets(0, 2)));
  text->SetMaxLines(15);
  container->AddChildView(text);
}

}  // namespace

#include "../../../../../../../chrome/browser/ui/views/permission_bubble/permission_prompt_impl.cc"  // NOLINT
