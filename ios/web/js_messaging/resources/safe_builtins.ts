// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

declare global {
  interface Window {
    webkit: any;
  }
}

class SafeBuiltins {
  readonly $Object: typeof Object = this.secureCopy(Object);
  readonly $Function: typeof Function = this.secureCopy(Function);
  readonly $Array: typeof Array = this.secureCopy(Array);
  readonly $ = function(value: any): any { return value; }

  // Sends a message to a script message handler in the browser
  readonly sendWebKitMessage: (handlerName: string, message: object|string) => void;

  // Sends a message to a script message handler in the browser and returns a
  // Promise that resolves when the browser replies
  readonly sendWebKitMessageWithReply: (handlerName: string, message: object|string) => Promise<any>;

  // Send a message expecting synchronously by using window.prompt and returns
  // the reply from the browser.
  readonly sendWebKitMessageSynchronously: (handlerName: string, message: object|string) => any|null;

  constructor() {
    // Setup private refs to capture in safe builtin functions
    const webkitMessageHandlers = window.webkit.messageHandlers;
    const windowPrompt = window.prompt.bind(window);
    const jsonStringify = JSON.stringify.bind(JSON);
    const jsonParse = JSON.parse.bind(JSON);

    this.sendWebKitMessage = (handlerName, message) => {
      webkitMessageHandlers[handlerName].postMessage(message);
    }

    this.sendWebKitMessageWithReply = (handlerName, message) => {
      return webkitMessageHandlers[handlerName].postMessage(message);
    }

    this.sendWebKitMessageSynchronously = (handlerName, message) => {
      const response = windowPrompt(jsonStringify({
        handler: handlerName,
        message: message
      }));
      if (!response) {
        return null;
      }
      try {
        return jsonParse(response)
      } catch {
        return null;
      }
    }

    // Freeze all the safe builtins and any function we export
    for (const value of [this.$Object, this.$Function, this.$Array, this.$,
      this.sendWebKitMessage, this.sendWebKitMessageWithReply]) {
      this.deepFreeze(value);
    }
  }

  // Freeze an object and its prototype
  private deepFreeze(value: any) {
    if (!value) {
      return;
    }
    this.$Object.freeze(value);
    const prototype = (value as any).prototype;
    if (prototype) {
      this.$Object.freeze(prototype);
    }
  }

  // Copies an object's signature to an object with no prototype to prevent
  // prototype polution attacks
  private secureCopy(value: any): any {
    let prototypeProperties = Object.create(null, value.prototype ?
      Object.getOwnPropertyDescriptors(value.prototype) : {});
    delete prototypeProperties['prototype'];

    let properties = Object.assign(
      Object.create(null),
      Object.getOwnPropertyDescriptors(value),
      value.prototype ? Object.getOwnPropertyDescriptors(value.prototype) :
        {}
    );

    // Do not copy the prototype.
    delete properties['prototype'];

    return new Proxy(Object.create(null, properties), {
      get(target, property, _receiver) {
        if (property == 'prototype') {
          return prototypeProperties;
        }
        return target[property];
      }
    });
  }
}

type SafeBuiltinsType = Window&(typeof globalThis)&{readonly __gSafeBuiltins: SafeBuiltins};

// Initializes window's `__gSafeBuiltins` property.
if (!(window as SafeBuiltinsType).__gSafeBuiltins) {
  Object.defineProperty(window, '__gSafeBuiltins', {
    value: Object.freeze(new SafeBuiltins()),
    writable: false,
    configurable: false,
    enumerable: false
  });
}

export const gSafeBuiltins: SafeBuiltins = (window as SafeBuiltinsType).__gSafeBuiltins;

// Export some shortcuts to items in SafeBuiltins
export const $Object = gSafeBuiltins.$Object;
export const $Function = gSafeBuiltins.$Function;
export const $Array = gSafeBuiltins.$Array;
export const $ = gSafeBuiltins.$;
