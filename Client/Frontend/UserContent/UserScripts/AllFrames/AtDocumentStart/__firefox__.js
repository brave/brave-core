/* vim: set ts=2 sts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

if (!window.__firefox__) {
  Object.defineProperty(window, "__firefox__", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: (function() {
        'use strict';
        
        let userScripts = {};
        let values = {};
        let includeOnce = function(userScript, initializer) {
            if (!userScripts[userScript]) {
              userScripts[userScript] = true;
              if (typeof initializer === 'function') {
                initializer();
              }
              return false;
            }

            return true;
        };
        
        let proxy = new Proxy({}, {
            get(target, property, receiver) {
                const descriptor = Object.getOwnPropertyDescriptor(target, property);
                if (descriptor && !descriptor.configurable && !descriptor.writable) {
                    return Reflect.get(target, property, receiver);
                }
                
                if (property === 'includeOnce') {
                    return includeOnce;
                }
                
                return Reflect.get(values, property, receiver);
            },
            
            set(target, name, value, receiver) {
                if (name === 'includeOnce') {
                    return false;
                }
                
                const descriptor = getOwnPropertyDescriptor(target, property);
                if (descriptor && !descriptor.configurable && !descriptor.writable) {
                    return false;
                }
                
                return Reflect.set(values, name, value, receiver);
            },
            
            defineProperty(target, property, descriptor) {
                if (descriptor && !descriptor.configurable) {
                    if (descriptor.set && !descriptor.get) {
                        return false;
                    }

                    if (!descriptor.writable) {
                        return Reflect.defineProperty(target, property, descriptor);
                    }
                }

                return Reflect.defineProperty(values, property, descriptor);
            },
            
            getOwnPropertyDescriptor(target, property) {
                const descriptor = Object.getOwnPropertyDescriptor(target, property);
                if (descriptor && !descriptor.configurable && !descriptor.writable) {
                    return descriptor;
                }
                
                return Object.getOwnPropertyDescriptor(values, property);
            },
            
            ownKeys(target) {
                var keys = [];
                keys = keys.concat(Object.keys(target));
                keys = keys.concat(Object.getOwnPropertyNames(target));
                return keys;
            }
        });
        
        return proxy;
    })()
  });
}
