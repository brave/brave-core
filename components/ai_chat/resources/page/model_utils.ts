// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../common/mojom'
import {
  ALL_RAIL_KEY,
  LOCAL_RAIL_KEY,
  OLLAMA_RAIL_KEY,
  PINNED_RAIL_KEY,
  getRailIcon,
} from '../common/model_rail_keys'

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

export function isOllamaModel(model: Mojom.Model) {
  return (
    model.options.customModelOptions?.endpoint.url === Mojom.OLLAMA_ENDPOINT
  )
}

export type RailEntry =
  | { key: string; label: string; icon: string; capability?: undefined }
  | {
      key: string
      label: string
      icon: string
      capability: Mojom.ModelCapability
    }

/**
 * Builds the filter rail: Pinned, All models, available capabilities,
 * Local (always), and Ollama when any Ollama models exist.
 */
export function getRailEntries(models: Mojom.Model[]): RailEntry[] {
  const entries: RailEntry[] = [
    {
      key: PINNED_RAIL_KEY,
      label: getLocale(S.CHAT_UI_PINNED_MODELS_LABEL),
      icon: getRailIcon(PINNED_RAIL_KEY),
    },
    {
      key: ALL_RAIL_KEY,
      label: getLocale(S.CHAT_UI_ALL_MODELS_LABEL),
      icon: getRailIcon(ALL_RAIL_KEY),
    },
  ]

  for (const capability of getAvailableModelCapabilities(models)) {
    entries.push({
      key: `capability-${capability}`,
      label: getModelCapabilityLabel(capability),
      icon: getModelCapabilityIcon(capability),
      capability,
    })
  }

  entries.push({
    key: LOCAL_RAIL_KEY,
    label: getLocale(S.CHAT_UI_LOCAL_MODELS_RAIL_LABEL),
    icon: getRailIcon(LOCAL_RAIL_KEY),
  })

  if (models.some(isOllamaModel)) {
    entries.push({
      key: OLLAMA_RAIL_KEY,
      label: getLocale(S.CHAT_UI_MODEL_OLLAMA_LABEL),
      icon: getRailIcon(OLLAMA_RAIL_KEY),
    })
  }

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
