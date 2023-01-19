/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/text_recognition_dialog_tracker.h"

TextRecognitionDialogTracker::TextRecognitionDialogTracker(
    content::WebContents* web_contents)
    : content::WebContentsUserData<TextRecognitionDialogTracker>(
          *web_contents) {}

TextRecognitionDialogTracker::~TextRecognitionDialogTracker() = default;

void TextRecognitionDialogTracker::SetActiveDialog(views::Widget* widget) {
  DCHECK(!active_dialog_ && !observation_.IsObserving());
  active_dialog_ = widget;
  observation_.Observe(widget);
}

void TextRecognitionDialogTracker::OnWidgetDestroying(views::Widget* widget) {
  DCHECK_EQ(active_dialog_, widget);
  DCHECK(observation_.IsObservingSource(widget));

  observation_.Reset();
  active_dialog_ = nullptr;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(TextRecognitionDialogTracker);
