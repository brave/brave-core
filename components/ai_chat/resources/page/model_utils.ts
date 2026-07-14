// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../common/mojom'
import {
  LOCAL_VENDOR_KEY,
  PINNED_VENDOR_KEY,
} from '../common/vendor_icon_map'
import { AUTOMATIC_MODEL_KEY } from '../common/constants'

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

export function isCustomModel(model: Mojom.Model) {
  return !!model.options.customModelOptions
}

/** Vendor key used for grouping: Local for custom models, else displayMaker. */
export function getModelVendorKey(model: Mojom.Model): string {
  if (isCustomModel(model)) {
    return LOCAL_VENDOR_KEY
  }
  const maker = model.options.leoModelOptions?.displayMaker
  return maker && maker.length > 0 ? maker : 'Brave'
}

export type VendorRailEntry = {
  key: string
  label: string
}

/**
 * Builds the vendor rail: Pinned first, then unique makers for Leo models,
 * then Local if any custom models exist (always include Local so empty state
 * can be shown).
 */
export function getVendorRailEntries(
  models: Mojom.Model[],
): VendorRailEntry[] {
  const makers = new Set<string>()
  let hasCustom = false
  for (const model of models) {
    if (isCustomModel(model)) {
      hasCustom = true
      continue
    }
    // Automatic has no maker; skip it for the vendor rail.
    if (model.key === AUTOMATIC_MODEL_KEY) {
      continue
    }
    makers.add(getModelVendorKey(model))
  }

  const entries: VendorRailEntry[] = [
    {
      key: PINNED_VENDOR_KEY,
      label: getLocale(S.CHAT_UI_PINNED_MODELS_LABEL),
    },
  ]

  // Stable-ish order based on appearance in the models list.
  const orderedMakers: string[] = []
  for (const model of models) {
    if (isCustomModel(model) || model.key === AUTOMATIC_MODEL_KEY) {
      continue
    }
    const key = getModelVendorKey(model)
    if (makers.has(key) && !orderedMakers.includes(key)) {
      orderedMakers.push(key)
    }
  }

  for (const maker of orderedMakers) {
    entries.push({ key: maker, label: maker })
  }

  // Always show Local so users can discover local-model empty state.
  entries.push({
    key: LOCAL_VENDOR_KEY,
    label: getLocale(S.CHAT_UI_LOCAL_MODELS_RAIL_LABEL),
  })
  void hasCustom

  return entries
}

export const ALL_MODEL_CAPABILITIES: Mojom.ModelCapability[] = [
  Mojom.ModelCapability.FAST,
  Mojom.ModelCapability.THINKING,
  Mojom.ModelCapability.SEARCH,
  Mojom.ModelCapability.VISION,
  Mojom.ModelCapability.TOOLS,
  Mojom.ModelCapability.AUDIO,
  Mojom.ModelCapability.VIDEO,
]

/** Capabilities present on at least one model, in ALL_MODEL_CAPABILITIES order. */
export function getAvailableModelCapabilities(
  models: Mojom.Model[],
): Mojom.ModelCapability[] {
  const present = new Set<Mojom.ModelCapability>()
  for (const model of models) {
    for (const capability of model.capabilities ?? []) {
      present.add(capability)
    }
  }
  return ALL_MODEL_CAPABILITIES.filter((capability) => present.has(capability))
}

export function getModelCapabilityLabel(
  capability: Mojom.ModelCapability,
): string {
  switch (capability) {
    case Mojom.ModelCapability.FAST:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_FAST)
    case Mojom.ModelCapability.THINKING:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_THINKING)
    case Mojom.ModelCapability.SEARCH:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_SEARCH)
    case Mojom.ModelCapability.VISION:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_VISION)
    case Mojom.ModelCapability.TOOLS:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_TOOLS)
    case Mojom.ModelCapability.AUDIO:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_AUDIO)
    case Mojom.ModelCapability.VIDEO:
      return getLocale(S.CHAT_UI_MODEL_CAPABILITY_VIDEO)
    default:
      return ''
  }
}

export function getModelCapabilityIcon(
  capability: Mojom.ModelCapability,
): string {
  switch (capability) {
    case Mojom.ModelCapability.FAST:
      return 'flash'
    case Mojom.ModelCapability.THINKING:
      return 'idea'
    case Mojom.ModelCapability.SEARCH:
      return 'search'
    case Mojom.ModelCapability.VISION:
      return 'eye-on'
    case Mojom.ModelCapability.TOOLS:
      return 'briefcase'
    case Mojom.ModelCapability.AUDIO:
      return 'volume-on'
    case Mojom.ModelCapability.VIDEO:
      return 'video-camera'
    default:
      return 'filter-settings'
  }
}

/** Formats capability chips as "Search · Vision · Tools". */
export function formatModelCapabilitiesSubtitle(
  capabilities: Mojom.ModelCapability[] | undefined,
): string {
  if (!capabilities?.length) {
    return ''
  }
  return capabilities.map(getModelCapabilityLabel).filter(Boolean).join(' · ')
}

export function modelHasAllCapabilities(
  model: Mojom.Model,
  required: Mojom.ModelCapability[],
): boolean {
  if (!required.length) {
    return true
  }
  const caps = model.capabilities ?? []
  return required.every((cap) => caps.includes(cap))
}
