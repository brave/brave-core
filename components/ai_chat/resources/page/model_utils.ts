// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../common/mojom'

export function isLeoModel(model: Mojom.Model) {
  return !!model.options.leoModelOptions
}

// Default soft client-side per-message input character limit. Leo models have
// their real context limits enforced server-side, so this only acts as a guard
// against pathologically large inputs.
export const DEFAULT_MAX_INPUT_CHAR = 20000

/**
 * Returns the maximum number of characters allowed in the input box for the
 * given model.
 *
 * For custom "Bring your own model" (BYOM) models the request is sent directly
 * to the user's own endpoint, so there is no reason to cap the input at the
 * default client-side limit. Instead the limit scales with the model's
 * configured context size (via its max associated content length), never
 * dropping below the default. See
 * https://github.com/brave/brave-browser/issues/49577.
 */
export function getMaxInputCharLimit(model?: Mojom.Model): number {
  const customModelOptions = model?.options.customModelOptions
  if (customModelOptions) {
    return Math.max(
      customModelOptions.maxAssociatedContentLength,
      DEFAULT_MAX_INPUT_CHAR,
    )
  }
  return DEFAULT_MAX_INPUT_CHAR
}

/**
 * Returns true if the model should appear in user-facing model selectors.
 * SUMMARY-category models are excluded (used internally, not user-selectable).
 */
export function isSelectableModel(model: Mojom.Model) {
  const category = model.options.leoModelOptions?.category
  if (category === undefined) return true
  return category !== Mojom.ModelCategory.SUMMARY
}

/**
 * Filters models to those suitable for user-facing selectors.
 * Excludes SUMMARY-category models.
 */
export function useSelectableModels(
  allModels: Mojom.Model[] | undefined,
): Mojom.Model[] {
  return React.useMemo(
    () => allModels?.filter(isSelectableModel) ?? [],
    [allModels],
  )
}
