/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PSST_CONSENT_DIALOG_TRACKER_H_
#define BRAVE_BROWSER_UI_VIEWS_PSST_CONSENT_DIALOG_TRACKER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

// Tracks whether psst consent dialog is active or not for WebContents.
class PsstConsentDialogTracker
    : public content::WebContentsUserData<PsstConsentDialogTracker>,
      public views::WidgetObserver {
 public:
  PsstConsentDialogTracker(const PsstConsentDialogTracker&) = delete;
  PsstConsentDialogTracker& operator=(const PsstConsentDialogTracker&) =
      delete;
  ~PsstConsentDialogTracker() override;

  void SetActiveDialog(views::Widget* widget);

  views::Widget* active_dialog() { return active_dialog_; }

 private:
  friend class content::WebContentsUserData<PsstConsentDialogTracker>;
  explicit PsstConsentDialogTracker(content::WebContents* web_contents);

  // views::WidgetObserver overrides
  void OnWidgetDestroying(views::Widget* widget) override;

  raw_ptr<views::Widget> active_dialog_ = nullptr;
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PSST_CONSENT_DIALOG_TRACKER_H_
