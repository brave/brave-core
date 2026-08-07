// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_TAB_INTERFACE_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_TAB_INTERFACE_H_

#include <memory>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/page_action/test_support/fake_tab_interface.h"
#include "components/tabs/public/tab_interface.h"

class TestingProfile;

namespace content {
class WebContents;
}

namespace page_actions {

// FakeTabInterface always reports the WebContents it was created with. This
// adds the ability to swap it, the way TabModel::DiscardContents() does, so
// that page action controllers can be tested across a contents swap.
class TestTabInterface : public FakeTabInterface {
 public:
  // Attaches the tab helpers the controller under test needs to a WebContents
  // swapped in by DiscardContents().
  using AttachTabHelpersCallback =
      base::RepeatingCallback<void(content::WebContents*)>;

  TestTabInterface(TestingProfile* profile,
                   AttachTabHelpersCallback attach_tab_helpers);
  ~TestTabInterface() override;

  // FakeTabInterface:
  base::CallbackListSubscription RegisterWillDiscardContents(
      WillDiscardContentsCallback callback) override;
  content::WebContents* GetContents() const override;

  // Swaps in a newly created WebContents, notifying observers while
  // GetContents() still returns the outgoing contents, as TabModel does.
  void DiscardContents();

 private:
  const raw_ptr<TestingProfile> profile_;
  const AttachTabHelpersCallback attach_tab_helpers_;

  // Every contents swapped in stays alive until this tab is destroyed, which
  // happens after anything observing them. Declared before |current_contents_|
  // so that it doesn't dangle while these are torn down.
  std::vector<std::unique_ptr<content::WebContents>> owned_contents_;

  raw_ptr<content::WebContents> current_contents_ = nullptr;

  base::RepeatingCallbackList<
      void(tabs::TabInterface*, content::WebContents*, content::WebContents*)>
      will_discard_contents_callbacks_;
};

}  // namespace page_actions

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_ACTION_TEST_TAB_INTERFACE_H_
