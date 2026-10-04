// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

declare global {
  interface Window {
    webkit: any
  }
}

class SafeBuiltins {
  readonly $Object: typeof Object = this.secureCopy(Object)
  readonly $Function: typeof Function = this.secureCopy(Function)
  readonly $Array: typeof Array = this.secureCopy(Array)
  readonly $ = function (value: any): any {
    return value
  }

  // Sends a message to a script message handler in the browser
  readonly sendWebKitMessage: (
    handlerName: string,
    message: object | string,
  ) => void

  // Sends a message to a script message handler in the browser and returns a
  // Promise that resolves when the browser replies
  readonly sendWebKitMessageWithReply: (
    handlerName: string,
    message: object | string,
  ) => Promise<any>

  // Send a message expecting synchronously by using window.prompt and returns
  // the reply from the browser.
  readonly sendWebKitMessageSynchronously: (
    handlerName: string,
    message: object | string,
  ) => any | null

  // Whether or not the window.location has a web scheme (http/https/data)
  readonly windowHasWebScheme: () => boolean

  // Prevents the modification of existing property attributes and values on,
  // and prevents the addition of new properties on both the value and its
  // prototype
  readonly deepFreeze: (value: any) => any

  // Recursively freezes an object, its prototype chain, and all of its
  // enumerable properties/values. `exceptions` lists constructor names whose
  // instances should be walked but left unfrozen.
  readonly recursiveFreeze: (obj: any, exceptions?: string[]) => any

  constructor() {
    // Setup private refs to capture in safe builtin functions
    const webkitMessageHandlers = window.webkit.messageHandlers
    const windowPrompt = window.prompt.bind(window)
    const jsonStringify = JSON.stringify.bind(JSON)
    const jsonParse = JSON.parse.bind(JSON)
    const windowProtocol = window.location.protocol
    const windowLocation = window.location.href

    this.windowHasWebScheme = () => {
      return (
        windowProtocol === 'https:'
        || windowProtocol === 'http:'
        || windowProtocol === 'data:'
      )
    }

    this.sendWebKitMessage = (handlerName, message) => {
      webkitMessageHandlers[handlerName].postMessage(message)
    }

    this.sendWebKitMessageWithReply = (handlerName, message) => {
      return webkitMessageHandlers[handlerName].postMessage(message)
    }

    const windowHasWebScheme = this.windowHasWebScheme
    this.sendWebKitMessageSynchronously = (handlerName, message) => {
      // Drop the request immediately if not running on a web page scheme, or local frame.
      // WebUI reserves its use of window.prompt for Mojo.
      if (
        !windowHasWebScheme()
        && !(
          windowLocation === 'about:blank' || windowLocation === 'about:srcdoc'
        )
      ) {
        return null
      }
      const response = windowPrompt(
        jsonStringify({
          handler: handlerName,
          message: message,
        }),
      )
      if (!response) {
        return null
      }
      try {
        return jsonParse(response)
      } catch {
        return null
      }
    }

    const $Object = this.$Object
    const $Array = this.$Array
    this.deepFreeze = (value: any): any => {
      if (!value) {
        return value
      }
      $Object.freeze(value)
      const prototype = (value as any).prototype
      if (prototype) {
        $Object.freeze(prototype)
      }
      return value
    }

    const recursiveFreeze = (obj: any, exceptions: string[] = []): any => {
      const primitiveTypes = $Array.of(
        'number',
        'string',
        'boolean',
        'null',
        'undefined',
      )
      const isIgnoredClass = (instance: any): boolean => {
        return (
          instance.constructor && exceptions.includes(instance.constructor.name)
        )
      }
      const freezeChild = (value: any) => {
        if (!value || primitiveTypes.includes(typeof value)) {
          return
        }
        if (value instanceof Object.getPrototypeOf(Uint8Array)) {
          return
        }
        recursiveFreeze(value, exceptions)
        if (!isIgnoredClass(value)) {
          $Object.freeze(value)
        }
      }

      // Do nothing to primitive types
      if (primitiveTypes.includes(typeof obj)) {
        return obj
      }

      if (!obj || (obj.constructor && obj.constructor.name === 'Object')) {
        return obj
      }

      // Do nothing to these prototypes
      if (obj === Object.prototype || obj === Function.prototype) {
        return obj
      }

      // Do nothing for typed arrays as they only contain primitives
      if (obj instanceof Object.getPrototypeOf(Uint8Array)) {
        return obj
      }

      if ($Array.isArray(obj) || obj instanceof Set) {
        for (const value of obj) {
          freezeChild(value)
        }
        return isIgnoredClass(obj) ? obj : $Object.freeze(obj)
      } else if (obj instanceof Map) {
        for (const value of obj.values()) {
          freezeChild(value)
        }
        return isIgnoredClass(obj) ? obj : $Object.freeze(obj)
      } else if (
        obj.constructor
        && (obj.constructor.name === 'Function'
          || obj.constructor.name === 'AsyncFunction'
          || obj.constructor.name === 'GeneratorFunction')
      ) {
        return $Object.freeze(obj)
      } else {
        const prototype = $Object.getPrototypeOf(obj)
        if (
          prototype
          && prototype !== Object.prototype
          && prototype !== Function.prototype
        ) {
          recursiveFreeze(prototype, exceptions)
          if (!isIgnoredClass(prototype)) {
            $Object.freeze(prototype)
          }
        }

        for (const value of $Object.values(obj)) {
          freezeChild(value)
        }

        for (const name of $Object.getOwnPropertyNames(obj)) {
          // Special handling for getters and setters because accessing them
          // will return the value, and not the function itself.
          const descriptor = $Object.getOwnPropertyDescriptor(obj, name)
          if (descriptor) {
            for (const value of $Array.of(
              descriptor.get,
              descriptor.set,
              descriptor.value,
            )) {
              freezeChild(value)
            }
            descriptor.enumerable = false
            descriptor.writable = false
            descriptor.configurable = false
            continue
          }

          freezeChild(obj[name])
        }

        return isIgnoredClass(obj) ? obj : $Object.freeze(obj)
      }
    }
    this.recursiveFreeze = recursiveFreeze

    // Freeze all the safe builtins and any function we export
    for (const value of [
      this.$Object,
      this.$Function,
      this.$Array,
      this.$,
      this.windowHasWebScheme,
      this.sendWebKitMessage,
      this.sendWebKitMessageWithReply,
      this.deepFreeze,
      this.recursiveFreeze,
    ]) {
      this.deepFreeze(value)
    }
  }

  // Copies an object's signature to an object with no prototype to prevent
  // prototype polution attacks
  private secureCopy(value: any): any {
    let prototypeProperties = Object.create(
      null,
      value.prototype ? Object.getOwnPropertyDescriptors(value.prototype) : {},
    )
    delete prototypeProperties['prototype']

    let properties = Object.assign(
      Object.create(null),
      Object.getOwnPropertyDescriptors(value),
      value.prototype ? Object.getOwnPropertyDescriptors(value.prototype) : {},
    )

    // Do not copy the prototype.
    delete properties['prototype']

    return new Proxy(Object.create(null, properties), {
      get(target, property, _receiver) {
        if (property == 'prototype') {
          return prototypeProperties
        }
        return target[property]
      },
    })
  }
}

type SafeBuiltinsType = Window
  & typeof globalThis & { readonly __gSafeBuiltins: SafeBuiltins }

// Initializes window's `__gSafeBuiltins` property.
if (!(window as SafeBuiltinsType).__gSafeBuiltins) {
  Object.defineProperty(window, '__gSafeBuiltins', {
    value: Object.freeze(new SafeBuiltins()),
    writable: false,
    configurable: false,
    enumerable: false,
  })
}

export const gSafeBuiltins: SafeBuiltins = (window as SafeBuiltinsType)
  .__gSafeBuiltins

// Export some shortcuts to items in SafeBuiltins
export const $Object = gSafeBuiltins.$Object
export const $Function = gSafeBuiltins.$Function
export const $Array = gSafeBuiltins.$Array
export const $ = gSafeBuiltins.$
