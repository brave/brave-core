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
    let properties = Object.assign({},
                                   Object.getOwnPropertyDescriptors(value),
                                   value.prototype ? Object.getOwnPropertyDescriptors(value.prototype) : undefined);
    
    // Do not copy the prototype.
    delete properties['prototype'];
    
    /// Making object not inherit from Object.prototype prevents prototype pollution attacks.
    return Object.create(null, properties);
  }
  
  /*
   *  Any objects that need to be secured must be done now
   */
  let $Object = secureCopy(Object);
  let $Function = secureCopy(Function);
  let $Reflect = secureCopy(Reflect);
  let $Array = secureCopy(Array);
  let $webkit = window.webkit;
  let $messageHandlers = $webkit.messageHandlers;
  
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
      
      for (const [name, property] of $Object.entries(overrides)) {
        if (($Object.getOwnPropertyDescriptor(toString, name) || {}).writable) {
          toString[name] = property;
        }

        $Object.defineProperty(toString, name, {
          enumerable: false,
          configurable: false,
          writable: false,
          value: property
        });

        if (name !== 'toString') {
          $.deepFreeze(toString[name]);
        }
      }

      $.deepFreeze(toString);

      for (const [name, property] of $Object.entries(overrides)) {
        if (($Object.getOwnPropertyDescriptor(value, name) || {}).writable) {
          value[name] = property;
        }

        $Object.defineProperty(value, name, {
          enumerable: false,
          configurable: false,
          writable: false,
          value: property
        });

        $.deepFreeze(value[name]);
      }
    }
    return value;
  };
  
  /*
   *  Freeze an object and its prototype
   */
  $.deepFreeze = function(value) {
    $Object.freeze(value);
    
    if (value.prototype) {
      $Object.freeze(value.prototype);
    }
    return value;
  };
    
  $.postNativeMessage = function(messageHandlerName, message) {
    let webkit = window.webkit;
    delete window.webkit;
    delete window.webkit.messageHandlers;
    delete $messageHandlers[messageHandlerName].postMessage;
    let result = $messageHandlers[messageHandlerName].postMessage(message);
    window.webkit = webkit;
    return result;
  }
  
  // Start securing functions before any other code can use them
  $($.deepFreeze);
  $($.postNativeMessage);
  $($);

  $.deepFreeze($.deepFreeze);
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
            $(fn)($, $Object, $Array);
          }
          return true;
        }

        return false;
      });
    
      let execute = $(function(fn) {
        if (typeof fn === 'function') {
          $(fn)($, $Object, $Array);
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
