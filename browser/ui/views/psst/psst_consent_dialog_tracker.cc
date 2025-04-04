/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/psst/psst_consent_dialog_tracker.h"

PsstConsentDialogTracker::PsstConsentDialogTracker(
    content::WebContents* web_contents)
    : content::WebContentsUserData<PsstConsentDialogTracker>(*web_contents) {}

PsstConsentDialogTracker::~PsstConsentDialogTracker() = default;

void PsstConsentDialogTracker::SetActiveDialog(views::Widget* widget) {
  observation_.Reset();
  active_dialog_ = widget;
  observation_.Observe(widget);
}

void PsstConsentDialogTracker::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(active_dialog_, widget);
  DCHECK(observation_.IsObservingSource(widget));

  observation_.Reset();
  active_dialog_ = nullptr;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PsstConsentDialogTracker);
