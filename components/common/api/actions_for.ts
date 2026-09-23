// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Extracts specified methods from an object and returns them as bound functions
 * in a plain object. This is useful for Mojo remote interfaces where methods
 * are on the prototype or accessed via proxy, making them non-enumerable and
 * thus not spreadable with `...obj`.
 *
 * @param source The object to extract methods from (e.g., a Mojo remote)
 * @param keys Array of method names to extract
 * @returns A plain object with the specified methods bound to the source
 *
 * @example
 * ```typescript
 * const exposedActions = actionsFor(conversationHandler, [
 *   'generateQuestions',
 *   'submitSummarizationRequest',
 * ] as const)
 * ```
 */
export function actionsFor<T, K extends keyof T>(
  source: T,
  keys: readonly K[],
): Pick<T, K> {
  const result = {} as Pick<T, K>
  for (const key of keys) {
    const val = source[key]
    if (typeof val === 'function') {
      ;(result as any)[key] = (val as Function).bind(source)
    }
  }
  return result
}
