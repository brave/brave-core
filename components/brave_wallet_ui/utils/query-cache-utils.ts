// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { FetchBaseQueryError } from '@reduxjs/toolkit/dist/query'

/**
 * Default tags used by the cacher helpers
 */
const defaultTags = ['UNAUTHORIZED', 'UNKNOWN_ERROR'] as const
type DefaultTags = typeof defaultTags[number]

function concatErrorCache<T, ID> (
  existingCache: CacheList<T, ID>,
  error: FetchBaseQueryError | undefined
): CacheList<T, ID> {
  if (error && 'status' in error && error.status === 401) {
    // unauthorized error
    return [...existingCache, 'UNAUTHORIZED']
  }

  // unknown error
  return [...existingCache, 'UNKNOWN_ERROR']
}

/**
 * An individual cache item
 */
export type CacheItem<T, ID> = { type: T, id: ID }

/**
 * A list of cache items, including a LIST entity cache
 */
export type CacheList<T, ID> = Array<
  | CacheItem<T, 'LIST'>
  | CacheItem<T, ID>
  | DefaultTags
>

/**
 * Inner function returned by `providesList` to be passed to the `provides` property of a query
 */
type InnerProvidesList<T> = <
  Results extends Array<{ id?: unknown }>,
  Error extends FetchBaseQueryError
>(
  results: Results | undefined,
  error: Error | undefined
) => CacheList<T, Results[number]['id']>

/**
 * HOF to create an entity cache to provide a LIST,
 * depending on the results being in a common format.
 *
 * Will not provide individual items without a result.
 *
 * @example
 * ```ts
 * const results = [
 *   { id: 1, message: 'foo' },
 *   { id: 2, message: 'bar' }
 * ]
 * providesList('Todo')(results)
 * // [
 * //   { type: 'Todo', id: 'List'},
 * //   { type: 'Todo', id: 1 },
 * //   { type: 'Todo', id: 2 },
 * // ]
 * ```
 */
export const providesList = <T extends string>(
  type: T
): InnerProvidesList<T> => (results, error) => {
  // is result available?
  if (results) {
    // successful query
    return [
      { type, id: 'LIST' },
      ...results.map((result) => ({ type, id: result.id } as const))
    ]
  }
  // Received an error, include an error cache item to the cache list
  return concatErrorCache([{ type, id: 'LIST' }], error)
}

/**
 * HOF to create an entity cache to invalidate a LIST.
 *
 * Invalidates regardless of result.
 *
 * @example
 * ```ts
 * invalidatesList('Todo')()
 * // [{ type: 'Todo', id: 'List' }]
 * ```
 */
export const invalidatesList = <T extends string>(type: T) => (): readonly [
  CacheItem<T, 'LIST'>
] => [{ type, id: 'LIST' }] as const

/**
 * HOF to create an entity cache for a single item using the query argument as the ID.
 *
 * @example
 * ```ts
 * cacheByIdArg('Todo')({ id: 5, message: 'walk the fish' }, undefined, 5)
 * // returns:
 * // [{ type: 'Todo', id: 5 }]
 * ```
 */
export const cacheByIdArg = <T extends string>(type: T) => <
  ID,
  Result = undefined,
  Error = undefined
>(
  result: Result,
  error: Error,
  id: ID
): readonly [CacheItem<T, ID>] => [{ type, id }] as const

/**
 * HOF to create an entity cache for a single item using the id property from the query argument as the ID.
 *
 * @example
 * ```ts
 * cacheByIdArgProperty('Todo')(undefined, { id: 5, message: 'sweep up' })
 * // returns:
 * // [{ type: 'Todo', id: 5 }]
 * ```
 */
export const cacheByIdArgProperty = <T extends string>(type: T) => <
  Arg extends { id: unknown },
  Result = undefined,
  Error = undefined
>(
  result: Result,
  error: Error,
  arg: Arg
): readonly [CacheItem<T, Arg['id']>] | [] => [{ type, id: arg.id }] as const

/**
 * HOF to invalidate the 'UNAUTHORIZED' type cache item.
 */
export const invalidatesUnauthorized = () => <
  Arg = undefined,
  Result = undefined,
  Error = undefined
>(
  result: Result,
  error: Error,
  arg: Arg
): ['UNAUTHORIZED'] => ['UNAUTHORIZED']

/**
 * HOF to invalidate the 'UNKNOWN_ERROR' type cache item.
 */
export const invalidatesUnknownErrors = () => <
  Arg = undefined,
  Result = undefined,
  Error = undefined
>(
  result: Result,
  error: Error,
  arg: Arg
): ['UNKNOWN_ERROR'] => ['UNKNOWN_ERROR']

/**
 * Utility helpers for common provides/invalidates scenarios
 */
export const cacher = {
  defaultTags,
  providesList,
  invalidatesList,
  cacheByIdArg,
  cacheByIdArgProperty,
  invalidatesUnauthorized,
  invalidatesUnknownErrors
}
