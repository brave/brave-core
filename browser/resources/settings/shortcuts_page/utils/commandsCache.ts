// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { EntityCachingWrapper } from '$web-common/mojomCache'
import {
  Command,
  CommandsServiceRemote,
  CommandsListenerInterface,
  CommandsListenerReceiver,
  CommandsService
} from 'gen/brave/components/commands/common/commands.mojom.m.js'

export const api = CommandsService.getRemote()

export class CommandsCache
  extends EntityCachingWrapper<Command>
  implements CommandsListenerInterface {
  private receiver = new CommandsListenerReceiver(this)
  private controller: CommandsServiceRemote

  constructor() {
    super()

    this.controller = api

    if (process.env.NODE_ENV !== 'test') {
      this.controller.addCommandsListener(
        this.receiver.$.bindNewPipeAndPassRemote()
      )
    }
  }

  assignAccelerator(
    commandId: number,
    accelerator: { codes: string; keys: string }
  ) {
    this.change({
      ...this.cache,
      [commandId]: {
        ...this.cache[commandId],
        accelerators: [...this.cache[commandId].accelerators, accelerator]
      }
    })
    this.controller.assignAcceleratorToCommand(commandId, accelerator.codes)
  }

  unassignAccelerator(commandId: number, accelerator: string) {
    const command = { ...this.cache[commandId] }
    command.accelerators = command.accelerators.filter(
      (a) => a.codes !== accelerator
    )
    this.change({
      ...this.cache,
      [commandId]: command
    })

    this.controller.unassignAcceleratorFromCommand(commandId, accelerator)
  }

  reset(commandId: number) {
    this.controller.resetAcceleratorsForCommand(commandId)
  }

  resetAll() {
    this.controller.resetAccelerators();
  }

  getKeyFromCode(code: string): Promise<string> {
    return this.controller.getKeyFromCode(code).then(c => c.key)
  }
}
