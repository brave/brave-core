/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_DELEGATE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_DELEGATE_H_

#include <string>

namespace speedreader {

enum class DistillationResult : int;

// SpeedreaderThrottleDelegate is an interface for the speedreader component to
// notify a tab_helper.
class SpeedreaderThrottleDelegate {
 public:
  virtual bool IsPageDistillationAllowed() = 0;
  virtual bool IsPageContentPresent() = 0;
  virtual std::string TakePageContent() = 0;
  virtual void OnDistillComplete(DistillationResult result) = 0;

 protected:
  virtual ~SpeedreaderThrottleDelegate() = default;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_THROTTLE_DELEGATE_H_
