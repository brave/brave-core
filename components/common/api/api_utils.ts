// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export type Closable<T = any> = T & {
  $: { close: () => void }
}

export type MethodKeys<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => (any|void) // it must be callable
    ? ReturnType<T[K]>
    : never
}[keyof T]

// Keys whose method's declared ReturnType is exactly void
export type VoidMethodKeys<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => any // it must be callable
    ? // and return void
      ReturnType<T[K]> extends void
      ? K
      : never
    : never
}[keyof T]

//
// Pick only those keys K of T where T[K] is (...args) => Promise<…>.
//
export type AsyncMethodKeys<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => Promise<any> ? K : never
}[keyof T]

export type ArgsOfMethod<T, K extends MethodKeys<T>> = T[K] extends (
  ...args: infer A
) => any
  ? A
  : never

export type ArgsOfAsyncMethod<T, K extends AsyncMethodKeys<T>> = T[K] extends (
  ...args: infer A
) => Promise<any>
  ? A
  : never

export type ResultOfAsyncMethod<
  T,
  K extends AsyncMethodKeys<T>,
> = T[K] extends (...args: any[]) => Promise<infer R> ? R : never

export function makeCloseable<T>(
  item: T,
  onClose: () => void = () => {},
): Closable<T> {
  return {
    ...item,
    $: {
      close: onClose,
    },
  }
}
