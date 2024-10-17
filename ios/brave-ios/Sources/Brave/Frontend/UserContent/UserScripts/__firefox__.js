// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

"use strict";

if (!window.__firefox__) {
  /*
   *  Generate toString
   */
  function generateToString(target, usingObjectDescriptor) {
    const toStringString = function() {
      return 'function toString() {\n    [native code]\n}';
    };

    const toString = function() {
      let functionDescription = `function ${ typeof target.name !== 'undefined' ? target.name : "" }() {\n    [native code]\n}`;
      if (usingObjectDescriptor) {
        return (typeof value === 'function') ? functionDescription : '[object Object]';
      }
      return functionDescription;
    };

    $Object.defineProperty(toString, 'name', {
      enumerable: false,
      configurable: true,
      writable: false,
      value: 'toString'
    });

    $Object.defineProperty(toStringString, 'name', {
      enumerable: false,
      configurable: true,
      writable: false,
      value: 'toString'
    });

    return [toString, toStringString];
  }

  /*
   *  Secure calls to `toString`
   */
  function secureToString(target, toString, toStringString, overrides = {}) {
    var fnOverrides = {...overrides};
    if ((target === toString || target === toStringString) && fnOverrides['toString']) {
      fnOverrides['toString'] = toStringString;
    }

    for (const [name, property] of $Object.entries(fnOverrides)) {
      let descriptor = $Object.getOwnPropertyDescriptor(target, name);
      if (!descriptor || descriptor.configurable) {
        $Object.defineProperty(target, name, {
          enumerable: false,
          configurable: false,
          writable: false,
          value: property
        });
      }

      descriptor = $Object.getOwnPropertyDescriptor(target, name);
      if (!descriptor || descriptor.writable) {
        fn[name] = property;
      }

      if (name !== 'toString') {
        $.deepFreeze(target[name]);
      }
    }

    //$.deepFreeze(toString);
  }

  /*
   *  Copies an object's signature to an object with no prototype to prevent prototype polution attacks
   */
  function secureCopy(value) {
    let prototypeProperties = Object.create(null, value.prototype ? Object.getOwnPropertyDescriptors(value.prototype) : undefined);
    delete prototypeProperties['prototype'];

    let properties = Object.assign(Object.create(null, undefined),
                                   Object.getOwnPropertyDescriptors(value),
                                   value.prototype ? Object.getOwnPropertyDescriptors(value.prototype) : undefined);

    // Do not copy the prototype.
    delete properties['prototype'];

    /// Making object not inherit from Object.prototype prevents prototype pollution attacks.
    //return Object.create(null, properties);

    // Create a Proxy so we can add an Object.prototype that has a null prototype and is read-only.
    return new Proxy(Object.create(null, properties), {
      get(target, property, receiver) {
        if (property == 'prototype') {
          return prototypeProperties;
        }

        if (property == 'toString') {
          let descriptor = $Object.getOwnPropertyDescriptor(target, property);
          if (descriptor && !descriptor.configurable && !descriptor.writable) {
            return Reflect.get(target, property);
          }

          const [toString, toStringString] = generateToString(target, false);

          const overrides = {
            'toString': toString,
            'call': $Function.call,
            'apply': $Function.apply,
            'bind': $Function.bind
          };

          secureToString(toStringString, toString, toStringString, overrides);
          $.deepFreeze(toStringString);

          secureToString(toString, toString, toStringString, overrides);
          $.deepFreeze(toString);
          return toString;
        }

        return target[property];
      }
    });
  }

  /*
   *  Any objects that need to be secured must be done now
   */
  let $Object = secureCopy(Object);
  let $Function = secureCopy(Function);
  let $Reflect = secureCopy(Reflect);
  let $Array = secureCopy(Array);

  secureCopy = undefined;
  let secureObjects = [$Object, $Function, $Reflect, $Array];

  /*
   *  Prevent recursive calls if a page overrides these.
   *  These functions can be frozen, without freezing the Function.prototype functions.
   */
  let call = $Function.call;
  let apply = $Function.apply;
  let bind = $Function.bind;

  call.call = call;
  call.apply = apply;

  apply.call = call;
  apply.apply = apply;

  bind.call = call;
  bind.apply = apply;


  /*
   *  Secures an object's attributes
   */
  let $ = function(value, overrideToString = true) {
    if ($Object.isExtensible(value)) {
      const [toString, toStringString] = generateToString(value, true);

      const overrides = overrideToString ? {
        'toString': toString
      } : {};

      if (typeof value === 'function') {
        const functionOverrides = {
          'call': $Function.call,
          'apply': $Function.apply,
          'bind': $Function.bind
        };

        for (const [key, value] of $Object.entries(functionOverrides)) {
          overrides[key] = value;
        }
      }

      // Secure our custom `toString`
      // Freeze our custom `toString`
      secureToString(toStringString, toString, toStringString, overrides);
      $.deepFreeze(toStringString);

      secureToString(toString, toString, toStringString, overrides);
      $.deepFreeze(toString);

      for (const [name, property] of $Object.entries(overrides)) {
        if (name == 'toString') {
          // Object.prototype.toString != Object.toString
          // They are two different functions, so we should check for both before overriding them
          if (value[name] && value[name] !== Object.prototype.toString && value[name] !== Object.toString) {
            // Secure the existing custom toString function
            // Do NOT deepFreeze existing toString functions
            // on custom objects we don't own. We secure it,
            // but not freeze it.
            // The object may want to change it or override it, etc.
            // Do not secure our already secured toString override,
            // if the custom override happens to be the same
            if (value[name] !== toString) {
              secureToString(value[name]);
            }
            continue;
          }

          // Object.prototype.toString != Object.toString
          // They are two different functions, so we should check for both before overriding them
          let descriptor = $Object.getOwnPropertyDescriptor(value, name);
          if (descriptor && descriptor.value && descriptor.value !== Object.prototype.toString && descriptor.value !== Object.toString) {
            // Secure the existing custom toString function
            // Do NOT deepFreeze existing toString functions
            // on custom objects we don't own. We secure it,
            // but not freeze it.
            // The object may want to change it or override it, etc.
            // Do not secure our already secured toString override,
            // if the custom override happens to be the same
            if (descriptor.value !== toString) {
              secureToString(descriptor.value);
            }
            continue;
          }

          // Object.prototype.toString != Object.toString
          // They are two different functions, so we should check for both before overriding them
          if (typeof value.toString !== 'undefined') {
            if (value.toString !== Object.prototype.toString && value.toString !== Object.toString) {
              if (value.toString !== toString) {
                secureToString(value.toString);
              }

              continue;
            }
          }
        }

        // Override all of the functions in the overrides array
        let descriptor = $Object.getOwnPropertyDescriptor(value, name);
        if (!descriptor || descriptor.configurable) {
          $Object.defineProperty(value, name, {
            enumerable: false,
            configurable: name == 'toString',
            writable: name == 'toString',
            value: property
          });
        }

        descriptor = $Object.getOwnPropertyDescriptor(value, name);
        if (!descriptor || descriptor.writable) {
          value[name] = property;
          $.deepFreeze(value[name]);
        }
      }
    }
    return value;
  };

  /*
   *  Freeze an object and its prototype
   */
  $.deepFreeze = function(value) {
    if (!value) {
      return value;
    }

    $Object.freeze(value);

    if (value.prototype) {
      $Object.freeze(value.prototype);
    }
    return value;
  };

  /*
   *  Freeze an object recursively
   */
  $.extensiveFreeze = function(obj, exceptions = []) {
    const primitiveTypes = $Array.of('number', 'string', 'boolean', 'null', 'undefined');
    const isIgnoredClass = function(instance) {
      return instance.constructor && exceptions.includes(instance.constructor.name);
    };

    // Do nothing to primitive types
    if (primitiveTypes.includes(typeof obj)) {
      return obj;
    }

    if (!obj || (obj.constructor && obj.constructor.name == "Object")) {
      return obj;
    }

    // Do nothing to these prototypes
    if (obj == Object.prototype || obj == Function.prototype) {
      return obj;
    }

    // Do nothing for typed arrays as they only contains primitives
    if (obj instanceof Object.getPrototypeOf(Uint8Array)) {
      return obj;
    }

    if ($Array.isArray(obj) || obj instanceof Set) {
      for (const value of obj) {
        if (!value || primitiveTypes.includes(typeof value)) {
          continue;
        }

        if (value instanceof Object.getPrototypeOf(Uint8Array)) {
          continue;
        }

        $.extensiveFreeze(value, exceptions);

        if (!isIgnoredClass(value)) {
          $Object.freeze($(value));
        }
      }

      return isIgnoredClass(obj) ? $(obj) : $Object.freeze($(obj));
    } else if (obj instanceof Map) {
      for (const value of obj.values()) {
        if (!value || primitiveTypes.includes(typeof value)) {
          continue;
        }

        if (value instanceof Object.getPrototypeOf(Uint8Array)) {
          continue;
        }

        $.extensiveFreeze(value, exceptions);

        if (!isIgnoredClass(value)) {
          $Object.freeze($(value));
        }
      }

      return isIgnoredClass(obj) ? $(obj) : $Object.freeze($(obj));
    } else if (obj.constructor && (obj.constructor.name == "Function" || obj.constructor.name == "AsyncFunction" || obj.constructor.name == "GeneratorFunction")) {
      return $Object.freeze($(obj));
    } else {
      let prototype = $Object.getPrototypeOf(obj);
      if (prototype && prototype != Object.prototype && prototype != Function.prototype) {
        $.extensiveFreeze(prototype, exceptions);

        if (!isIgnoredClass(prototype)) {
          $Object.freeze($(prototype));
        }
      }

      for (const value of $Object.values(obj)) {
        if (!value || primitiveTypes.includes(typeof value)) {
          continue;
        }

        if (value instanceof Object.getPrototypeOf(Uint8Array)) {
          continue;
        }

        $.extensiveFreeze(value, exceptions);

        if (!isIgnoredClass(value)) {
          $Object.freeze($(value));
        }
      }

      for (const name of $Object.getOwnPropertyNames(obj)) {
        // Special handling for getters and setters because accessing them will return the value,
        // and not the function itself.
        let descriptor = $Object.getOwnPropertyDescriptor(obj, name);
        if (descriptor) {
          let values = $Array.of(descriptor.get, descriptor.set, descriptor.value);
          for (const value of values) {
            if (!value || primitiveTypes.includes(typeof value) || value instanceof Object.getPrototypeOf(Uint8Array)) {
              continue;
            }

            $.extensiveFreeze(value, exceptions);

            if (!isIgnoredClass(value)) {
              $Object.freeze($(value));
            }
          }

          descriptor.enumerable = false;
          descriptor.writable = false;
          descriptor.configurable = false;
          continue;
        }

        let value = obj[name];
        if (!value || primitiveTypes.includes(typeof value)) {
          continue;
        }

        if (value instanceof Object.getPrototypeOf(Uint8Array)) {
          continue;
        }

        $.extensiveFreeze(value, exceptions);

        if (!isIgnoredClass(value)) {
          $Object.freeze($(value));
        }
      }

      return isIgnoredClass(obj) ? $(obj) : $Object.freeze($(obj));
    }
  };

  $.postNativeMessage = function(messageHandlerName, message) {
    if (!window.webkit || !window.webkit.messageHandlers) {
      return Promise.reject(new TypeError("undefined is not an object (evaluating 'webkit.messageHandlers')"));
    }

    return new Promise((resolve, reject) => {
      var oldWebkit = window.webkit;
      delete window['webkit'];
      
      // WebKit no longer restores the handler immediately! So we poll for when that happens and resolve the promise accordingly.
      const timeout = 5000;
      let startTime = Date.now();
      
      // While loop blocks synchronously. SetTimeout or SetInterval can cause a race condition.
      while(true) {
        if (window.webkit.messageHandlers && window.webkit.messageHandlers[messageHandlerName]) {
          let result = window.webkit.messageHandlers[messageHandlerName].postMessage(message);
          window.webkit = oldWebkit;
          result.then(resolve).catch(reject);
          break;
        } else if (Date.now() - startTime >= timeout) {
          reject(new TypeError("undefined is not an object (evaluating 'webkit.messageHandlers')"));
          break;
        }
      }
    });
  };

  $.dispatchEvent = function(event) {
    delete window.dispatchEvent;
    let originalDispatchEvent = window.dispatchEvent(event);
    return originalDispatchEvent;
  }

  $.addEventListener = function(type, listener, optionsOrUseCapture) {
    delete window.addEventListener;
    let originalAddEventListener = window.addEventListener(type, listener, optionsOrUseCapture);
    return originalAddEventListener;
  }

  // Start securing functions before any other code can use them
  $($.deepFreeze);
  $($.extensiveFreeze);
  $($.postNativeMessage);
  $($.dispatchEvent);
  $($.addEventListener);
  $($);

  $.deepFreeze($.deepFreeze);
  $.deepFreeze($.extensiveFreeze);
  $.deepFreeze($.postNativeMessage);
  $.deepFreeze($.dispatchEvent);
  $.deepFreeze($.addEventListener);
  $.deepFreeze($);

  for (const value of secureObjects) {
    $(value);
    $.deepFreeze(value);
  }

  /*
   *  Creates a Proxy object that does the following to all objects using it:
   *  - Symbols are not printable or accessible via `toString`
   *  - Symbols are not enumerable
   *  - Symbols are read-only
   *  - Symbols are not configurable
   *  - Symbols can be completely hidden via `hiddenProperties`
   *  - All child properties and objects follow the above rules as well
   */
  let createProxy = $(function(hiddenProperties) {
    let values = $({});
    return new Proxy({}, {
      apply(target, thisArg, argumentsList) {
        return $Reflect.apply(target, thisArg, argumentsList);
      },

      deleteProperty(target, property) {
        if (property in target) {
          delete target[property];
        }

        if (property in values) {
          delete target[property];
        }
      },

      get(target, property, receiver) {
        if (hiddenProperties && hiddenProperties[property]) {
          return hiddenProperties[property];
        }

        const descriptor = $Reflect.getOwnPropertyDescriptor(target, property);
        if (descriptor && !descriptor.configurable && !descriptor.writable) {
          return $Reflect.get(target, property, receiver);
        }

        return $Reflect.get(values, property, receiver);
      },

      set(target, name, value, receiver) {
        if (hiddenProperties && hiddenProperties[name]) {
          return false;
        }

        const descriptor = $Reflect.getOwnPropertyDescriptor(target, name);
        if (descriptor && !descriptor.configurable && !descriptor.writable) {
          return false;
        }

        if (value) {
          value = $(value);
        }

        return $Reflect.set(values, name, value, receiver);
      },

      defineProperty(target, property, descriptor) {
        if (descriptor && !descriptor.configurable) {
          if (descriptor.set && !descriptor.get) {
            return false;
          }

          if (descriptor.value) {
            descriptor.value = $(descriptor.value);
          }

          if (!descriptor.writable) {
            return $Reflect.defineProperty(target, property, descriptor);
          }
        }

        if (descriptor.value) {
          descriptor.value = $(descriptor.value);
        }

        return $Reflect.defineProperty(values, property, descriptor);
      },

      getOwnPropertyDescriptor(target, property) {
        const descriptor = $Reflect.getOwnPropertyDescriptor(target, property);
        if (descriptor && !descriptor.configurable && !descriptor.writable) {
          return descriptor;
        }

        return $Reflect.getOwnPropertyDescriptor(values, property);
      },

      ownKeys(target) {
        let keys = [];
        /*keys = keys.concat(Object.keys(target));
        keys = keys.concat(Object.getOwnPropertyNames(target));*/
        keys = keys.concat($Reflect.ownKeys(target));
        return keys;
      }
    });
  });

  /*
   *  Creates window.__firefox__ with a `Proxy` object as defined above
   */
  $Object.defineProperty(window, "__firefox__", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: ($(function() {
      'use strict';

      let userScripts = $({});
      let includeOnce = $(function(name, fn) {
        if (!userScripts[name]) {
          userScripts[name] = true;
          if (typeof fn === 'function') {
            $(fn)($, $Object, $Function, $Array);
          }
          return true;
        }

        return false;
      });

      let execute = $(function(fn) {
        if (typeof fn === 'function') {
          $(fn)($, $Object, $Function, $Array);
          return true;
        }
        return false;
      });

      return createProxy({'includeOnce': $.deepFreeze(includeOnce), 'execute': $.deepFreeze(execute)});
    }))()
  });

  $.deepFreeze(UserMessageHandler);
  $.deepFreeze(webkit.messageHandlers);
}
