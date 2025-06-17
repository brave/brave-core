// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ArgsOfMethod, MethodKeys } from './api_utils'
import { EventDef, event } from './create_interface_api'

export function partialReceiver<
  Interface extends {},
  Receiver,
  const PartialInterface extends Partial<Interface>,
>(
  Type: new () => Interface,
  Receiver: new (handler: Interface) => Receiver,
  handler: PartialInterface,
) {
  const observer = Object.create(Type.prototype) as Interface
  Object.assign(observer, handler)
  return new Receiver(observer)
}

export function eventsFor<
  Interface extends {},
  const PartialInterface extends Partial<Interface>,
>(
  Type: new () => Interface,
  /**
   * Partial interface that contains only the methods that are to be observed
   */
  handler: PartialInterface,
  /**
   * Function that provides the constructed receiver to the caller so that it
   * can be bound.
   */
  observerHandler: (observer: Interface) => void,
) {
  const observer = Object.create(Type.prototype) as Interface
  Object.assign(observer, handler)

  type ValidKeys = Extract<keyof PartialInterface, MethodKeys<Interface>>

  // TODO: only bubble up events that are asked for. Some events may just want to be
  // handled in the API definition so that all events are handled in the same place.
  type Events = {
    [K in keyof PartialInterface]: EventDef<
      [],
      PartialInterface[K] extends (...args: any) => any
        ? Parameters<PartialInterface[K]>
        : never
    >
  }

  // Create 'events' that can be used by createInterfaceAPI
  const events = {} as Events

  for (const key of Object.keys(handler) as ValidKeys[]) {
    const originalHandler = (observer as any)[key]
    let apiEventEmitter: (args: ArgsOfMethod<Interface, typeof key>) => void
    const newHandler = (...args: ArgsOfMethod<Interface, typeof key>) => {
      // Call the original handler with the arguments
      ;(originalHandler as Function).call(observer, ...args)
      // Emit the event with the arguments
      if (apiEventEmitter) {
        ;(apiEventEmitter! as any)(...(args as unknown as []))
      } else {
        console.warn('event emitter was not provided for event', key)
      }
    }
    // Original is new
    ;(observer as any)[key] = newHandler

    // Handle each event and fire to the API emitter
    ;(events as any)[key] = event((emitter) => {
      // register new handler for the event, so we can fire on the API
      apiEventEmitter = emitter
    })
  }

  // Allow the caller to bind the receiver
  observerHandler(observer)

  return events
}
