/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {Command, CommandsEvent, CommandsListenerInterface,
  CommandsListenerReceiver, CommandsService, CommandsServiceRemote}
  from '../commands.mojom-webui.js'

type CacheListener = (
  newValue: Record<string, Command>,
  oldValue: Record<string, Command>) => void

export interface CommandOption {
  name: string
  value: number
}

// A read-only live command list (name, id); not imported from elsewhere,
// since the shared caching base lives behind a path alias unavailable here.
export class CommandsCache implements CommandsListenerInterface {
  private cache: Record<string, Command> = {}
  private listeners: CacheListener[] = []
  private receiver = new CommandsListenerReceiver(this)
  private controller: CommandsServiceRemote

  constructor() {
    this.controller = CommandsService.getRemote()
    this.controller.addCommandsListener(
      this.receiver.$.bindNewPipeAndPassRemote())
  }

  addListener(listener: CacheListener, init = true) {
    this.listeners.push(listener)
    if (init) listener(this.cache, {})
  }

  removeListener(listener: CacheListener) {
    this.listeners = this.listeners.filter(
      (existingListener) => existingListener !== listener)
  }

  changed(event: CommandsEvent) {
    const oldValue = this.cache
    const newValue = {...oldValue}
    for (const id in event.addedOrUpdated) {
      newValue[id] = event.addedOrUpdated[id]
    }
    for (const id of event.removed) delete newValue[id]

    this.cache = newValue
    for (const listener of this.listeners) listener(newValue, oldValue)
  }
}
