// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { TabSearchItemElement } from './tab_search_item-chromium.js'

// Upstream renders chrome-internal tab URLs with a hardcoded 'chrome://'
// scheme prefix that it prepends to the secondary text in `dataChanged_`.
// Brave displays such URLs with the 'brave://' scheme everywhere else (the
// location bar, tab hover cards, etc.) via
// `brave_utils::ReplaceChromeToBraveScheme`, but that C++ display formatting
// never runs for this WebUI dialog: the page handler sends the raw GURL and
// the scheme prefix is produced entirely in TypeScript. Patch `dataChanged_`
// so the prefix shown here matches the rest of the browser. The tab's
// underlying URL keeps the real chrome:// scheme used for navigation.
const proto = TabSearchItemElement.prototype as unknown as
    { dataChanged_: (this: TabSearchItemElement) => void }
const originalDataChanged = proto.dataChanged_

proto.dataChanged_ = function (this: TabSearchItemElement) {
  originalDataChanged.call(this)
  const schemeNode = this.$.secondaryTextInner.firstChild
  if (schemeNode && schemeNode.nodeType === Node.TEXT_NODE &&
      schemeNode.textContent === 'chrome://') {
    schemeNode.textContent = 'brave://'
  }
}

export * from './tab_search_item-chromium.js'
