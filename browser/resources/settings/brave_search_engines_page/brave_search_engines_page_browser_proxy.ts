// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js'

export interface BraveSearchEnginesPageBrowserProxy {
  getPrivateSearchEnginesList(): Promise<any[]> // TODO(petemill): Define the expected type
  setDefaultPrivateSearchEngine(modelIndex: number): void
}
 
export class BraveSearchEnginesPageBrowserProxyImpl implements BraveSearchEnginesPageBrowserProxy {
  getPrivateSearchEnginesList() {
    return sendWithPromise('getPrivateSearchEnginesList')
  }
  setDefaultPrivateSearchEngine(modelIndex: number) {
    chrome.send('setDefaultPrivateSearchEngine', [modelIndex])
  }
  static getInstance() {
    return instance || (instance = new BraveSearchEnginesPageBrowserProxyImpl())
  }
}

let instance: BraveSearchEnginesPageBrowserProxy|null = null
