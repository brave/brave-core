// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { MiddlewareAPI, Dispatch, Middleware, AnyAction } from 'redux';

type Handlers = Map<string, Function>

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

  on<T>(actionType: string, doStuff: (store: MiddlewareAPI, payload: T) => void) {
    this.handlersByType.set(actionType, doStuff)
  }

  middleware: Middleware = (store: MiddlewareAPI) => (next: Dispatch<AnyAction>) => (action: AnyAction) => {
    const result = next(action)
    const doStuff = this.handlersByType.get(action.type)
    if (doStuff) {
      doStuff(store, next, action)
    }
    return result
  }
}