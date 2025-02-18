/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { SDKModel } from '../../core/sdk/SDKModel.js'
import { type Target } from '../../core/sdk/Target.js'

export interface BraveDispatcher {
  braveEventReceived(event: ProtocolBraveEvent): void
}

export class BraveModel
  extends SDKModel<EventTypes>
  implements BraveDispatcher
{
  readonly agent: any
  readonly #events: Set<string>
  #enabled: boolean = false

  constructor(target: Target, events: Set<string>) {
    super(target)

    this.agent = (target as any).braveAgent()
    ;(target as any).registerBraveDispatcher(this as any)
    this.#events = events
    this.enable()
  }

  enable(): void {
    if (this.#enabled) {
      return
    }
    void this.agent.invoke_enable()
    this.#enabled = true
  }

  disable(): void {
    if (!this.#enabled) {
      return
    }
    this.#enabled = false
    void this.agent.invoke_disable()
  }

  braveEventReceived(event: ProtocolBraveEvent): void {
    if (!this.#events.has(event.event)) {
      return
    }

    this.dispatchEventToListeners(Events.BRAVE_EVENT_RECEIVED, {
      braveModel: this,
      event: event
    })
  }
}

export const enum Events {
  BRAVE_EVENT_RECEIVED = 'BraveEventReceived'
}

export interface ProtocolBraveEvent {
  event: string
  params: any
}

export interface BraveEvent {
  braveModel: BraveModel
  event: ProtocolBraveEvent
}

export type EventTypes = {
  [Events.BRAVE_EVENT_RECEIVED]: BraveEvent
}
