/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TEXT_RECOGNITION_DIALOG_TRACKER_H_
#define BRAVE_BROWSER_UI_VIEWS_TEXT_RECOGNITION_DIALOG_TRACKER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

// Tracks whether text recognition dialog is active or not for WebContents.
class TextRecognitionDialogTracker
    : public content::WebContentsUserData<TextRecognitionDialogTracker>,
      public views::WidgetObserver {
 public:
  TextRecognitionDialogTracker(const TextRecognitionDialogTracker&) = delete;
  TextRecognitionDialogTracker& operator=(const TextRecognitionDialogTracker&) =
      delete;
  ~TextRecognitionDialogTracker() override;

  void SetActiveDialog(views::Widget* widget);

  views::Widget* active_dialog() { return active_dialog_; }

 private:
  friend class content::WebContentsUserData<TextRecognitionDialogTracker>;
  explicit TextRecognitionDialogTracker(content::WebContents* web_contents);

  // views::WidgetObserver overrides
  void OnWidgetDestroying(views::Widget* widget) override;

  raw_ptr<views::Widget> active_dialog_ = nullptr;
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TEXT_RECOGNITION_DIALOG_TRACKER_H_
