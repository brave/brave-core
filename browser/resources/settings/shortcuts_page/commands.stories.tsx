// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { App, setCommandsApi } from './commands'
import { Meta } from '@storybook/react'
import { createCommandsApi } from './utils/commandsApi'
import * as Mojom from 'gen/brave/components/commands/common/commands.mojom.m'
import { mockMojoReceiver } from '$web-common/mockMojoReceiver'

mockMojoReceiver(Mojom.CommandsListenerReceiver)

const mockAPI = createCommandsApi({
  addCommandsListener: (listener: Mojom.CommandsListenerInterface) => {
    listener.changed({
      removed: [],
      addedOrUpdated: {
        '1': {
          id: '1',
          name: 'Command 1',
          description: 'Command 1 description',
          accelerators: [
            { keys: "Ctrl+1", codes: "Ctrl+1" },
            { keys: "Alt+Space", codes: "Alt+Space" },
            { keys: "Ctrl+Alt+Space", codes: "Ctrl+Alt+Space" }
          ]
        }
      }
    })
  },
  resetAccelerators: () => { },
  unassignAcceleratorFromCommand: () => { },
  assignAcceleratorToCommand: () => { },
  getKeyFromCode: (code) => Promise.resolve({ key: code }),
  resetAcceleratorsForCommand: () => { }
})

setCommandsApi(mockAPI)

export const _Commands = () => {
  return <App />
}

export default {
  title: 'Settings/Shortcuts/Commands',
  component: App,
} as Meta<typeof App>
