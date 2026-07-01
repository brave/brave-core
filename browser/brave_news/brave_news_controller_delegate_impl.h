// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_DELEGATE_IMPL_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"

class Profile;

namespace brave_news {

// Browser-layer implementation of BraveNewsController::Delegate. Lives here
// rather than in components/ because opening browser UI requires
// //chrome/browser/ui.
class BraveNewsControllerDelegateImpl : public BraveNewsController::Delegate {
 public:
  explicit BraveNewsControllerDelegateImpl(Profile* profile);
  ~BraveNewsControllerDelegateImpl() override;

  BraveNewsControllerDelegateImpl(const BraveNewsControllerDelegateImpl&) =
      delete;
  BraveNewsControllerDelegateImpl& operator=(
      const BraveNewsControllerDelegateImpl&) = delete;

  // BraveNewsController::Delegate:
  void OpenSettings() override;

 private:
  const raw_ref<Profile> profile_;
};

}  // namespace brave_news

#endif  // BRAVE_BROWSER_BRAVE_NEWS_BRAVE_NEWS_CONTROLLER_DELEGATE_IMPL_H_
