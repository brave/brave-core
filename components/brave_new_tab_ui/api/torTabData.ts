// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export type TorTabData = {
  torCircuitEstablished: boolean,
  torInitProgress: string
}

type TorTabDataUpdatedHandler = (data: TorTabData) => void

export function getTorTabData (): Promise<TorTabData> {
  return window.cr.sendWithPromise<TorTabData>('getNewTabPageTorProperties')
}

export function addChangeListener (listener: TorTabDataUpdatedHandler): void {
  window.cr.addWebUIListener('tor-tab-data-updated', listener)
}
