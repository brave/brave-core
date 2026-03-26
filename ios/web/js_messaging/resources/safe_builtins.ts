// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict';

export interface SecureFunction {
  (value: any, overrideToString?: boolean): any;
  deepFreeze<T>(value: T): T;
  extensiveFreeze<T>(obj: T, exceptions?: string[]): T;
  dispatchEvent(event: Event): boolean;
  addEventListener(
    type: string,
    listener: EventListenerOrEventListenerObject,
    optionsOrUseCapture?: boolean | AddEventListenerOptions
  ): void;
  windowOrigin: string;
}

function generateToString(
  target: any,
  usingObjectDescriptor: boolean
): [() => string, () => string] {
  const toStringString = function() {
    return 'function toString() {\n    [native code]\n}';
  };

  const toString = function() {
    const name =
      typeof target.name !== 'undefined' ? target.name : '';
    const desc = `function ${name}() {\n    [native code]\n}`;
    if (usingObjectDescriptor) {
      return (typeof target === 'function') ? desc : '[object Object]';
    }
    return desc;
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

function secureToString(
  target: any,
  toString?: any,
  toStringString?: any,
  overrides: Record<string, any> = {}
): void {
  const fnOverrides = {...overrides};
  if (
    (target === toString || target === toStringString) &&
    fnOverrides['toString']
  ) {
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
      target[name] = property;
    }

    if (name !== 'toString') {
      deepFreeze(target[name]);
    }
  }
}

function secureCopy(value: any): any {
  const prototypeProperties = Object.create(
    null,
    value.prototype
      ? Object.getOwnPropertyDescriptors(value.prototype)
      : undefined
  );
  delete prototypeProperties['prototype'];

  const properties = Object.assign(
    Object.create(null, undefined),
    Object.getOwnPropertyDescriptors(value),
    value.prototype
      ? Object.getOwnPropertyDescriptors(value.prototype)
      : undefined
  );
  delete properties['prototype'];

  return new Proxy(Object.create(null, properties), {
    get(target: any, property: string | symbol, receiver: any) {
      if (property === 'prototype') {
        return prototypeProperties;
      }

      if (property === 'toString') {
        const descriptor =
          Object.getOwnPropertyDescriptor(target, property);
        if (
          descriptor &&
          !descriptor.configurable &&
          !descriptor.writable
        ) {
          return Reflect.get(target, property);
        }

        const [ts, tss] = generateToString(target, false);
        const overrides: Record<string, any> = {
          'toString': ts,
          'call': $Function.call,
          'apply': $Function.apply,
          'bind': $Function.bind
        };

        secureToString(tss, ts, tss, overrides);
        deepFreeze(tss);
        secureToString(ts, ts, tss, overrides);
        deepFreeze(ts);
        return ts;
      }

      return target[property];
    }
  });
}

function deepFreeze<T>(value: T): T {
  if (!value) {
    return value;
  }
  Object.freeze(value);
  if ((value as any).prototype) {
    Object.freeze((value as any).prototype);
  }
  return value;
}

export const $Object: typeof Object = secureCopy(Object);
export const $Function: typeof Function = secureCopy(Function);
export const $Reflect: typeof Reflect = secureCopy(Reflect);
export const $Array: typeof Array = secureCopy(Array);

const secureObjects = [$Object, $Function, $Reflect, $Array];

const call: typeof Function.prototype.call = $Function.call;
const apply: typeof Function.prototype.apply = $Function.apply;
const bind: typeof Function.prototype.bind = $Function.bind;

(call as any).call = call;
(call as any).apply = apply;
(apply as any).call = call;
(apply as any).apply = apply;
(bind as any).call = call;
(bind as any).apply = apply;

function secure(value: any, overrideToString: boolean = true): any {
  if ($Object.isExtensible(value)) {
    const [toString, toStringString] = generateToString(value, true);

    const overrides: Record<string, any> =
      overrideToString ? {'toString': toString} : {};

    if (typeof value === 'function') {
      const functionOverrides: Record<string, any> = {
        'call': $Function.call,
        'apply': $Function.apply,
        'bind': $Function.bind
      };
      for (const [key, val] of $Object.entries(functionOverrides)) {
        overrides[key] = val;
      }
    }

    secureToString(toStringString, toString, toStringString, overrides);
    deepFreeze(toStringString);
    secureToString(toString, toString, toStringString, overrides);
    deepFreeze(toString);

    for (const [name, property] of $Object.entries(overrides)) {
      if (name === 'toString') {
        if (
          value[name] &&
          value[name] !== Object.prototype.toString &&
          value[name] !== Object.toString
        ) {
          if (value[name] !== toString) {
            secureToString(value[name]);
          }
          continue;
        }

        let descriptor = $Object.getOwnPropertyDescriptor(value, name);
        if (
          descriptor &&
          descriptor.value &&
          descriptor.value !== Object.prototype.toString &&
          descriptor.value !== Object.toString
        ) {
          if (descriptor.value !== toString) {
            secureToString(descriptor.value);
          }
          continue;
        }

        if (typeof value.toString !== 'undefined') {
          if (
            value.toString !== Object.prototype.toString &&
            value.toString !== Object.toString
          ) {
            if (value.toString !== toString) {
              secureToString(value.toString);
            }
            continue;
          }
        }
      }

      let descriptor = $Object.getOwnPropertyDescriptor(value, name);
      if (!descriptor || descriptor.configurable) {
        $Object.defineProperty(value, name, {
          enumerable: false,
          configurable: name === 'toString',
          writable: name === 'toString',
          value: property
        });
      }

      descriptor = $Object.getOwnPropertyDescriptor(value, name);
      if (!descriptor || descriptor.writable) {
        value[name] = property;
        deepFreeze(value[name]);
      }
    }
  }
  return value;
}

export const $: SecureFunction = secure as SecureFunction;

$.deepFreeze = deepFreeze;

$.extensiveFreeze = function<T>(obj: T, exceptions: string[] = []): T {
  const primitiveTypes = $Array.of(
    'number', 'string', 'boolean', 'null', 'undefined'
  );
  const isIgnoredClass = function(instance: any): boolean {
    return instance.constructor &&
      exceptions.includes(instance.constructor.name);
  };

  if (primitiveTypes.includes(typeof obj)) {
    return obj;
  }

  if (
    !obj ||
    ((obj as any).constructor &&
      (obj as any).constructor.name === 'Object')
  ) {
    return obj;
  }

  if (
    (obj as any) === Object.prototype ||
    (obj as any) === Function.prototype
  ) {
    return obj;
  }

  if (obj instanceof Object.getPrototypeOf(Uint8Array)) {
    return obj;
  }

  if ($Array.isArray(obj) || obj instanceof Set) {
    for (const value of obj as any) {
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
    return isIgnoredClass(obj)
      ? $(obj) : ($Object.freeze($(obj)) as T);
  } else if (obj instanceof Map) {
    for (const value of (obj as Map<any, any>).values()) {
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
    return isIgnoredClass(obj)
      ? $(obj) : ($Object.freeze($(obj)) as T);
  } else if (
    (obj as any).constructor && (
      (obj as any).constructor.name === 'Function' ||
      (obj as any).constructor.name === 'AsyncFunction' ||
      (obj as any).constructor.name === 'GeneratorFunction'
    )
  ) {
    return $Object.freeze($(obj)) as T;
  } else {
    const prototype = $Object.getPrototypeOf(obj);
    if (
      prototype &&
      prototype !== Object.prototype &&
      prototype !== Function.prototype
    ) {
      $.extensiveFreeze(prototype, exceptions);
      if (!isIgnoredClass(prototype)) {
        $Object.freeze($(prototype));
      }
    }

    for (const value of $Object.values(obj as object)) {
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

    for (const name of $Object.getOwnPropertyNames(obj as object)) {
      const descriptor =
        $Object.getOwnPropertyDescriptor(obj as object, name);
      if (descriptor) {
        const values = $Array.of(
          descriptor.get, descriptor.set, descriptor.value
        );
        for (const value of values) {
          if (
            !value ||
            primitiveTypes.includes(typeof value) ||
            value instanceof Object.getPrototypeOf(Uint8Array)
          ) {
            continue;
          }
          $.extensiveFreeze(value, exceptions);
          if (!isIgnoredClass(value)) {
            $Object.freeze($(value));
          }
        }
        continue;
      }

      const value = (obj as any)[name];
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

    return isIgnoredClass(obj)
      ? $(obj) : ($Object.freeze($(obj)) as T);
  }
};

$.dispatchEvent = function(event: Event): boolean {
  delete (window as any).dispatchEvent;
  return window.dispatchEvent(event);
};

$.addEventListener = function(
  type: string,
  listener: EventListenerOrEventListenerObject,
  optionsOrUseCapture?: boolean | AddEventListenerOptions
): void {
  delete (window as any).addEventListener;
  window.addEventListener(type, listener, optionsOrUseCapture);
};

$.windowOrigin = Object.freeze(window.origin) as string;
$($.windowOrigin);
deepFreeze($.windowOrigin as any);

$($.deepFreeze);
$($.extensiveFreeze);
$($.postNativeMessage);
$($.dispatchEvent);
$($.addEventListener);
$($);

deepFreeze($.deepFreeze as any);
deepFreeze($.extensiveFreeze as any);
deepFreeze($.postNativeMessage as any);
deepFreeze($.dispatchEvent as any);
deepFreeze($.addEventListener as any);
deepFreeze($);

for (const value of secureObjects) {
  $(value);
  deepFreeze(value);
}
