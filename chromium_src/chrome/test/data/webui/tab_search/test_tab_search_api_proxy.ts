// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type { PageHandlerInterface } from 'chrome://tab-search.top-chrome/tab_search.js'

import { TestTabSearchApiProxy } from './test_tab_search_api_proxy-chromium.js'

// The proxy implements the generated `PageHandlerInterface`, which includes
// the methods Brave adds to the `PageHandler` mojom.
type BravePageHandlerMethods = Pick<
  PageHandlerInterface,
  | 'getSuggestedTopics'
  | 'getFocusTabs'
  | 'undoFocusTabs'
  | 'openLeoGoPremiumPage'
  | 'openLearnMorePage'
  | 'setTabFocusEnabled'
  | 'getTabFocusShowFRE'
  | 'searchTabsByContent'
>

declare module './test_tab_search_api_proxy-chromium.js' {
  interface TestTabSearchApiProxy extends BravePageHandlerMethods {}
}

Object.assign(TestTabSearchApiProxy.prototype, {
  getSuggestedTopics: () => Promise.resolve({ topics: [], error: null }),
  getFocusTabs: () => Promise.resolve({ windowCreated: false, error: null }),
  undoFocusTabs: () => Promise.resolve(),
  openLeoGoPremiumPage: () => {},
  openLearnMorePage: () => {},
  setTabFocusEnabled: () => {},
  getTabFocusShowFRE: () => Promise.resolve({ showFRE: false }),
  searchTabsByContent: () => Promise.resolve({ tabIds: [] }),
} satisfies BravePageHandlerMethods)

export * from './test_tab_search_api_proxy-chromium.js'
