/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

declare module 'react-chrome-redux' {
  interface Action {
    type: any
  }

  interface AnyAction extends Action {
    [extraProps: string]: any
  }

  interface Unsubscribe {
    (): void
  }

  type Reducer<S> = (state: S, action: AnyAction) => S

  const wrapStore: (store: object, options?: {
    portName?: string
    dispatchResponder?: any
  }) => void

  class Store<S> {
    constructor (options: {
      portName: string
      state?: object
      extensionId?: string
    })

    ready (callBack?: () => void): Promise<boolean>

    dispatch (data: object): Promise<object>

    getState (): S

    replaceState (newState: S): void

    patchState (partialState: Partial<S>): void

    subscribe (listener: () => void): Unsubscribe

    // this is missing in the original react chrome redux implementatio, but we need it in redux
    replaceReducer (nextReducer: Reducer<S>): void
  }
}
