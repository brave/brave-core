/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_RESULT_DELEGATE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_RESULT_DELEGATE_H_

#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}  // namespace content

// SpeedreaderResultDelegate is an interface for the speedreader component to
// notify a tab_helper that the distillation has completed. To meet the
// requirements of a tab_helper, the interface is scoped to the lifetime of
// WebContents.
class SpeedreaderResultDelegate {
 public:
  virtual void OnDistillComplete(bool success) = 0;
};

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_RESULT_DELEGATE_H_
