/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"

class ActiveWindowSearchProviderManager;

class BraveBrowserFrame : public BrowserFrame {
 public:
  explicit BraveBrowserFrame(BrowserView* browser_view);
  ~BraveBrowserFrame() override;
  BraveBrowserFrame(const BraveBrowserFrame&) = delete;
  BraveBrowserFrame& operator=(const BraveBrowserFrame&) = delete;

  // BrowserFrame overrides:
  const ui::NativeTheme* GetNativeTheme() const override;

 private:
  friend class SearchEngineProviderServiceTest;

  FRIEND_TEST_ALL_PREFIXES(SearchEngineProviderServiceTest,
                           CheckTorWindowSearchProviderTest);

  BrowserView* view_;
  std::unique_ptr<ActiveWindowSearchProviderManager> search_provider_manager_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_FRAME_H_
