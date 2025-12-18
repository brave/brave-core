// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createInterfaceApi,
  eventsFor,
  endpointsFor,
  state,
} from '$web-common/api'

import * as Mojom from 'gen/brave/components/commands/common/commands.mojom.m'

type Actions = Pick<
  Mojom.CommandsServiceInterface,
  | 'resetAccelerators'
  | 'resetAcceleratorsForCommand'
>

export function createCommandsApi(
  controller: Mojom.CommandsServiceInterface,
) {
  const api = createInterfaceApi({
    actions: {
      commands: controller as Actions,
    },
    endpoints: {
      ...endpointsFor(controller,
        {
          getKeyFromCode: {
            response: (result) => result.key,
          },
          unassignAcceleratorFromCommand: {
            mutationResponse() { },
            onMutate([commandId, accelerator]) {
              // Eagerly update the cache to remove the accelerator
              api.endpoints.commands.update(result => {
                const update = { ...result }
                update[commandId].accelerators = update[commandId].accelerators.filter(a => a.codes !== accelerator)
                return update
              })
            }
          },
          assignAcceleratorToCommand: {
            mutationResponse() { },
            onMutate([commandId, accelerator]) {
              // Eagerly update the cache with the new accelerator
              api.endpoints.commands.update(result => {
                const update = {
                  ...result,
                  [commandId]: {
                    ...result[commandId],
                    accelerators: [...result[commandId].accelerators, accelerator]
                  }
                }
                return update
              })
            }
          }
        }),
      commands: state<{ [commandId: string]: Mojom.Command }>({})
    },
    events: {
      ...eventsFor(Mojom.CommandsListenerInterface, {
        changed(event) {
          api.endpoints.commands.update(result => {
            const update = { ...result }
            for (const removed of event.removed) {
              delete update[removed]
            }
            for (const [id, changed] of Object.entries(event.addedOrUpdated) as [string, Mojom.Command][]) {
              update[id] = changed
            }
            return update
          })
        }
      }, observer => {
        controller.addCommandsListener(new Mojom.CommandsListenerReceiver(observer).$.bindNewPipeAndPassRemote())
      })
    }
  })

  return api
}
