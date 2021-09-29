// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import { ConnectionState } from './types/connection_state'
export default class APIProxy {
  static getInstance: () => APIProxy
  showUI: () => {}
  closeUI: () => {}
  getConnectionState: () => Promise<StateResponse>
  createVPNConnection: () => {}
  connect: () => {}
  disconnect: () => {}
  addVPNObserver: (obj: ServiceObserver) => {}
}
interface ServiceObserver {
  onConnectionCreated?: Function
  onConnectionRemoved?: Function
  onConnectionStateChanged?: Function
}
interface StateResponse {
  state: ConnectionState
}
