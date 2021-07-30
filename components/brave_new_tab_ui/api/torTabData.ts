// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Cr from '../../common/cr'

export type TorTabData = {
  torCircuitEstablished: boolean,
  torInitProgress: string
}

type TorTabDataUpdatedHandler = (data: TorTabData) => void

export function getTorTabData (): Promise<TorTabData> {
  return Cr.sendWithPromise<TorTabData>('getNewTabPageTorProperties')
}

export function addChangeListener (listener: TorTabDataUpdatedHandler): void {
  Cr.addWebUIListener('tor-tab-data-updated', listener)
}
