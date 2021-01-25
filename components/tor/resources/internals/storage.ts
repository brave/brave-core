/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const defaultState: TorInternals.State = {
  generalInfo: {
    torVersion: '',
    torPid: -1,
    torProxyURI: '',
    isTorConnected: false,
    torInitPercentage: ''
  },
  log: '',
  torControlEvents: []
}

export const load = (): TorInternals.State => {
  return defaultState
}
