// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, Middleware, AnyAction } from 'redux'

type Handler<T> = (store: MiddlewareAPI, payload: T) => void
type Handlers = Map<string, Array<Handler<unknown>>>

/**
 * Quick n easy redux middleware generator for async actions.
 * Usage example:
 * ```
 * const asyncActions = new AsyncHandler()
 * asyncActions.on<MyActionPayloadType>('action-type', ({ getState, dispatch }, payload) => {
 *    // do something with payload, getState, and dispatch
 *    // like call an async API, etc
 * }
 * Redux.createStore(
 *   myReducer,
 *   applyMiddleware(asyncActions.middleware)
 * )
 * ```
 */
export default class AsyncHandler {
  handlersByType: Handlers = new Map()

  setHandlerForAction<T> (actionType: string, doStuff: Handler<T>) {
    let handlers = this.handlersByType.get(actionType)
    if (!handlers) {
      handlers = []
      this.handlersByType.set(actionType, handlers)
    }
    handlers.push(doStuff)
  }

  on<T> (actionType: string | string[], doStuff: Handler<T>) {
    if (Array.isArray(actionType)) {
      for (const action of actionType) {
        this.setHandlerForAction(action, doStuff)
      }
    } else {
      this.setHandlerForAction(actionType, doStuff)
    }
  }

  middleware: Middleware = (store: MiddlewareAPI) => (next: Dispatch<AnyAction>) => (action: AnyAction) => {
    const result = next(action)
    const handlers = this.handlersByType.get(action.type)
    if (handlers) {
      for (const handler of handlers) {
        handler(store, action.payload)
      }
    }
    return result
  }
}
