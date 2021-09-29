/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Client, ClientListener } from './interfaces'

export const ClientContext = React.createContext({} as Client)

// A helper hook for listening to host state changes
export function useClientListener (client: Client, listener: ClientListener) {
  React.useEffect(() => client.addListener(listener), [client])
}
