// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Cr from '../../common/cr'

//
// Manages get and set of NTP private data
// Ensures everything to do with communication
// with the WebUI backend is all in 1 place,
// especially string keys.
//

export type PrivateTabData = {
  useAlternativePrivateSearchEngine: boolean
}

type PrivateTabDataUpdatedHandler = (data: PrivateTabData) => void

export function getPrivateTabData (): Promise<PrivateTabData> {
  return Cr.sendWithPromise<PrivateTabData>('getNewTabPagePrivateProperties')
}

export function toggleAlternativePrivateSearchEngine (): void {
  chrome.send('toggleAlternativePrivateSearchEngine', [])
}

export function addChangeListener (listener: PrivateTabDataUpdatedHandler): void {
  Cr.addWebUIListener('private-tab-data-updated', listener)
}
