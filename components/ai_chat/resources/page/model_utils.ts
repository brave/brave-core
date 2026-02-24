// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../common/mojom'

export function isLeoModel(model: Mojom.Model) {
  return !!model.options.leoModelOptions
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
