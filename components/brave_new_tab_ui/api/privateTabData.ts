// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Cr from 'chrome://resources/js/cr.js'

//
// Manages get and set of NTP private data
// Ensures everything to do with communication
// with the WebUI backend is all in 1 place,
// especially string keys.
//

export type PrivateTabData = {
  useAlternativePrivateSearchEngine: boolean
  showAlternativePrivateSearchEngineToggle: boolean
}

type PrivateTabDataUpdatedHandler = (data: PrivateTabData) => void

export function getPrivateTabData (): Promise<PrivateTabData> {
  return Cr.sendWithPromise('getNewTabPagePrivateProperties')
}

export function toggleAlternativePrivateSearchEngine (): void {
  chrome.send('toggleAlternativePrivateSearchEngine', [])
}

export function addChangeListener (listener: PrivateTabDataUpdatedHandler): void {
  Cr.addWebUiListener('private-tab-data-updated', listener)
}
