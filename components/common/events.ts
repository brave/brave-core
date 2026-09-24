// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

type EventsStorage = {
  [eventName: string]: Array<Listener<unknown>>
}

type Listener<T> = (arg: T) => any

export default class Events {
  private handlers: EventsStorage = {}

  addEventListener<T> (eventName: string, listener: Listener<T>) {
    let listeners = this.handlers[eventName]
    if (!listeners) {
      listeners = this.handlers[eventName] = []
    }
    this.handlers[eventName].push(listener)
  }

  removeEventListener (eventName: string, listener: Function) {
    const listeners = this.handlers[eventName]
    if (!listeners) {
      return
    }
    const index = listeners.findIndex(i => i === listener)
    if (index < 0) {
      return
    }
    listeners.splice(index, 1)
  }

  dispatchEvent<T> (eventName: string, args: T) {
    const listeners = this.handlers[eventName]
    if (!listeners) {
      return
    }
    for (const listener of listeners) {
      listener(args)
    }
  }
}
