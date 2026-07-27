/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_DELEGATE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_DELEGATE_H_

#include <string>

class GURL;

namespace speedreader {

enum class DistillationResult : int;

// SpeedreaderDelegate is an interface for the speedreader component to
// notify a tab_helper.
class SpeedreaderDelegate {
 public:
  virtual bool IsPageDistillationAllowed() = 0;
  // Returns true if the distilled content is available and it was distilled
  // from |url|. The content must not be sent as a body of any other url.
  virtual bool IsPageContentPresent(const GURL& url) = 0;
  virtual std::string TakePageContent() = 0;
  virtual void OnDistillComplete(DistillationResult result) = 0;
  virtual void OnDistilledDocumentSent() = 0;

 protected:
  virtual ~SpeedreaderDelegate() = default;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_DELEGATE_H_
