// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

"use strict";

if (!window.__firefox__) {
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
  let $webkit = window.webkit;
  let $MessageHandlers = $webkit.messageHandlers;
  
  secureCopy = undefined;
  let secureObjects = [$Object, $Function, $Reflect, $Array, $MessageHandlers];
  
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
  let $ = function(value) {
    if ($Object.isExtensible(value)) {
      const description = (typeof value === 'function') ?
                          `function () {\n\t[native code]\n}` :
                          '[object Object]';
      
      const toString = function() {
        return description;
      };
      
      const overrides = {
        'toString': toString
      };
      
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
      
      // Secure calls to `toString`
      const secureToString = function(toString) {
        for (const [name, property] of $Object.entries(overrides)) {
          let descriptor = $Object.getOwnPropertyDescriptor(toString, name);
          if (!descriptor || descriptor.configurable) {
            $Object.defineProperty(toString, name, {
              enumerable: false,
              configurable: false,
              writable: false,
              value: property
            });
          }
          
          descriptor = $Object.getOwnPropertyDescriptor(toString, name);
          if (!descriptor || descriptor.writable) {
            toString[name] = property;
          }
          
          if (name !== 'toString') {
            $.deepFreeze(toString[name]);
          }
        }

        $.deepFreeze(toString);
      };
      
      // Secure our custom `toString`
      secureToString(toString);

      for (const [name, property] of $Object.entries(overrides)) {
        if (name == 'toString') {
          // Object.prototype.toString != Object.toString
          // They are two different functions, so we should check for both before overriding them
          if (value[name] && value[name] !== Object.prototype.toString && value[name] !== Object.toString) {
            // Secure the existing custom toString function
            secureToString(value[name]);
            continue;
          }
          
          // Object.prototype.toString != Object.toString
          // They are two different functions, so we should check for both before overriding them
          let descriptor = $Object.getOwnPropertyDescriptor(value, name);
          if (descriptor && descriptor.value !== Object.prototype.toString && descriptor.value !== Object.toString) {
            // Secure the existing custom toString function
            secureToString(value[name]);
            continue;
          }
        }
        
        // Override all of the functions in the overrides array
        let descriptor = $Object.getOwnPropertyDescriptor(value, name);
        if (!descriptor || descriptor.configurable) {
          $Object.defineProperty(value, name, {
            enumerable: false,
            configurable: false,
            writable: false,
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
    } else if (obj.constructor && (obj.constructor.name == "Function" || obj.constructor.name == "AsyncFunction")) {
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
    
    let webkit = window.webkit;
    delete window.webkit.messageHandlers[messageHandlerName].postMessage;
    delete window.webkit.messageHandlers[messageHandlerName];
    delete window.webkit.messageHandlers;
    delete window.webkit;
    let result = $MessageHandlers[messageHandlerName].postMessage(message);
    window.webkit = webkit;
    return result;
  };
  
  // Start securing functions before any other code can use them
  $($.deepFreeze);
  $($.extensiveFreeze);
  $($.postNativeMessage);
  $($);

  $.deepFreeze($.deepFreeze);
  $.deepFreeze($.extensiveFreeze);
  $.deepFreeze($.postNativeMessage);
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
