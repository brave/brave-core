// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: This proxy makes all methods resolve after a microTask to ensure that all the mojom methods are asynchronous (not being async can violate assumptions about mojo).
const delayResultProxy = <T extends object>(impl: T) => {
  return new Proxy(impl, {
    get(target, prop) {
      console.log('get', target, prop)
      const value = target[prop as keyof T]
      if (typeof value !== 'function') {
        return value;
      }

      return (...args: any[]) => {
        const {promise, resolve} = Promise.withResolvers()
        queueMicrotask(() => {
          resolve(value(...args))
        })
        return promise
      }
    }
  })
}

// This function overrides bindNewPipeAndPassRemote to return a fake remote interface which directly calls the impl of the receiver.
export function mockMojoReceiver<T extends { prototype: any }>(receiver: T) {
  // First, we need to override the $ property to 
  // replace bindNewPipeAndPassRemote with a function that returns the implementation.
  Object.defineProperty(receiver.prototype, '$', {
    get() {
      const self = this
      const delayedImpl = delayResultProxy(self.impl)
      return {
        bindNewPipeAndPassRemote: () => {
          console.log('bindNewPipeAndPassRemote', delayedImpl)
          return delayedImpl
        }
      }
    },
  
    set() {
      // do nothing we're just preventing mojom from initializing normally
    },
    configurable: true,
  })
}
