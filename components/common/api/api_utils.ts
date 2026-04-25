// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Wraps a type with a `$` property containing a `close` method.
 *
 * This type is designed to be compatible with mojom interfaces so that we can
 * signal when the client connection should be closed. It can be useful for any type of
 * persistent connection, e.g. websocket or mojo remote.
 *
 * @template T - The base type to wrap
 *
 * @example
 * ```ts
 * interface MyService {
 *   getData(): Promise<string>
 * }
 *
 * function createService(): Closable<MyService> {
 *   const remote = new MyServiceRemote()
 *   return makeCloseable(remote, () => remote.close())
 * }
 *
 * const service = createService()
 * const data = await service.getData()
 * service.$.close() // Clean up the connection
 * ```
 */
export type Closable<T = any> = T & {
  $: { close: () => void }
}

/**
 * Extracts the keys of all methods (callable properties) from a type.
 *
 * Unlike `keyof T` which includes all properties, this filters to only
 * those keys whose values are functions. This is useful when you need to
 * operate on or constrain to only the callable members of an interface.
 *
 * @template T - The type containing methods
 *
 * @example
 * ```ts
 * interface MyApi {
 *   getName(): string
 *   getAge(): number
 *   data: boolean // Not a method, ignored
 * }
 *
 * type Methods = MethodKeys<MyApi> // 'getName' | 'getAge'
 * ```
 */
export type MethodKeys<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => any | void ? K : never
}[keyof T]

/**
 * Extracts the keys of methods whose return type is exactly `void`.
 *
 * This is useful for identifying "fire-and-forget" methods in an interface,
 * such as event handlers or notification methods that don't return a value.
 * Unlike a simple `keyof T`, this filters to only methods (not properties)
 * and only those that return void.
 *
 * @template T - The type containing methods
 *
 * @example
 * ```ts
 * interface MyApi {
 *   fetchData(): Promise<string>
 *   notifyChange(): void
 *   reset(): void
 *   count: number
 * }
 *
 * type VoidMethods = VoidMethodKeys<MyApi> // 'notifyChange' | 'reset'
 * ```
 */
export type VoidMethodKeys<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => any // it must be callable
    ? // and return void
      ReturnType<T[K]> extends void
      ? K
      : never
    : never
}[keyof T]

/**
 * Extracts the keys of methods that return a `Promise`.
 *
 * This is useful for identifying async methods in an interface, particularly
 * for mojom-generated interfaces where query methods return promises. Unlike
 * `keyof T`, this filters to only async methods, excluding sync methods,
 * void methods, and non-method properties.
 *
 * @template T - The type containing methods
 *
 * @example
 * ```ts
 * interface MyApi {
 *   fetchData(): Promise<string>
 *   getCount(): Promise<number>
 *   reset(): void
 *   name: string
 * }
 *
 * type AsyncMethods = AsyncMethodKeys<MyApi> // 'fetchData' | 'getCount'
 * ```
 */
export type AsyncMethodKeys<T> = {
  [K in keyof T]: T[K] extends (...args: any[]) => Promise<any> ? K : never
}[keyof T]

/**
 * Extracts the parameter types of a specific method from a type.
 *
 * Similar to TypeScript's built-in `Parameters<T>`, but works with a method
 * key on an interface rather than a standalone function type. This is useful
 * when building wrappers or proxies around interface methods.
 *
 * @template T - The type containing the method
 * @template K - The key of the method (must be a valid method key from `MethodKeys<T>`)
 *
 * @example
 * ```ts
 * interface MyApi {
 *   updateUser(id: number, name: string): void
 * }
 *
 * type UpdateArgs = ArgsOfMethod<MyApi, 'updateUser'> // [id: number, name: string]
 *
 * function wrapUpdate(...args: ArgsOfMethod<MyApi, 'updateUser'>) {
 *   console.log('Updating:', args)
 *   return api.updateUser(...args)
 * }
 * ```
 */
export type ArgsOfMethod<T, K extends MethodKeys<T>> = T[K] extends (
  ...args: infer A
) => any
  ? A
  : never

/**
 * Extracts the parameter types of a specific async method from a type.
 *
 * A specialized version of `ArgsOfMethod` that only works with async methods
 * (those returning a `Promise`). This provides better type safety when you
 * specifically need to work with async method parameters.
 *
 * @template T - The type containing the method
 * @template K - The key of the async method (must be a valid key from `AsyncMethodKeys<T>`)
 *
 * @example
 * ```ts
 * interface MyApi {
 *   fetchUser(id: number): Promise<User>
 *   syncReset(): void
 * }
 *
 * type FetchArgs = ArgsOfAsyncMethod<MyApi, 'fetchUser'> // [id: number]
 *
 * // This would be a type error:
 * // type ResetArgs = ArgsOfAsyncMethod<MyApi, 'syncReset'>
 * ```
 */
export type ArgsOfAsyncMethod<T, K extends AsyncMethodKeys<T>> = T[K] extends (
  ...args: infer A
) => Promise<any>
  ? A
  : never

/**
 * Extracts the unwrapped return type of an async method (the type inside the `Promise`).
 *
 * Similar to TypeScript's `Awaited<ReturnType<T>>`, but works with a method key
 * on an interface. This is particularly useful for mojom interfaces where you
 * need to work with the actual data type returned by an async query method.
 *
 * @template T - The type containing the method
 * @template K - The key of the async method (must be a valid key from `AsyncMethodKeys<T>`)
 *
 * @example
 * ```ts
 * interface MyApi {
 *   fetchUser(id: number): Promise<{ name: string; age: number }>
 *   fetchItems(): Promise<string[]>
 * }
 *
 * type UserResult = ResultOfAsyncMethod<MyApi, 'fetchUser'>
 * // { name: string; age: number }
 *
 * type ItemsResult = ResultOfAsyncMethod<MyApi, 'fetchItems'>
 * // string[]
 * ```
 */
export type ResultOfAsyncMethod<
  T,
  K extends AsyncMethodKeys<T>,
> = T[K] extends (...args: any[]) => Promise<infer R> ? R : never

/**
 * Wraps an item with a `$` property containing a `close` method.
 *
 * This factory function creates a `Closable<T>` by spreading the original
 * item's properties and adding the cleanup mechanism. Use this for non-mojo,
 * persistent connections that need explicit cleanup.
 *
 * @template T - The type of the item to wrap
 * @param item - The object to make closeable
 * @param onClose - Optional callback invoked when `$.close()` is called
 * @returns The item wrapped with a `$` property containing the close method
 *
 * @example
 * ```ts
 * const remote = new MyConnection()
 * const service = makeCloseable(remote, () => {
 *.  MyLibrary.CloseConnection(remote)
 *   console.log('Connection closed')
 * })
 *
 * // Use the service
 * await service.fetchData()
 *
 * // Clean up when done
 * service.$.close()
 * ```
 */
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
