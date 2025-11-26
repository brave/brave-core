/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_IGNORE_ONBEFOREUNLOAD_WEB_CONTENTS_DELEGATE_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_IGNORE_ONBEFOREUNLOAD_WEB_CONTENTS_DELEGATE_H_

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents_delegate.h"

class IgnoreOnBeforeUnloadWebContentsDelegate
    : public content::WebContentsDelegate {
 public:
  explicit IgnoreOnBeforeUnloadWebContentsDelegate(
      TabStripModel* tab_strip_model);
  IgnoreOnBeforeUnloadWebContentsDelegate(
      const IgnoreOnBeforeUnloadWebContentsDelegate&) = delete;
  IgnoreOnBeforeUnloadWebContentsDelegate& operator=(
      const IgnoreOnBeforeUnloadWebContentsDelegate&) = delete;
  ~IgnoreOnBeforeUnloadWebContentsDelegate() override;

  void CloseContents(content::WebContents* source) override;
  bool ShouldSuppressDialogs(content::WebContents* source) override;

 private:
  raw_ptr<TabStripModel> tab_strip_model_;
};

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_IGNORE_ONBEFOREUNLOAD_WEB_CONTENTS_DELEGATE_H_
