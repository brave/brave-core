// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/test_tab_interface.h"

#include <utility>

#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"

namespace page_actions {

TestTabInterface::TestTabInterface(TestingProfile* profile,
                                   AttachTabHelpersCallback attach_tab_helpers)
    : FakeTabInterface(profile),
      profile_(profile),
      attach_tab_helpers_(std::move(attach_tab_helpers)) {
  attach_tab_helpers_.Run(FakeTabInterface::GetContents());
}

TestTabInterface::~TestTabInterface() = default;

base::CallbackListSubscription TestTabInterface::RegisterWillDiscardContents(
    WillDiscardContentsCallback callback) {
  return will_discard_contents_callbacks_.Add(std::move(callback));
}

content::WebContents* TestTabInterface::GetContents() const {
  return current_contents_ ? current_contents_.get()
                           : FakeTabInterface::GetContents();
}

void TestTabInterface::DiscardContents() {
  auto contents = content::WebContents::Create(
      content::WebContents::CreateParams(profile_));
  attach_tab_helpers_.Run(contents.get());
  will_discard_contents_callbacks_.Notify(this, GetContents(), contents.get());
  current_contents_ = contents.get();
  owned_contents_.push_back(std::move(contents));
}

}  // namespace page_actions
