// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import * as Mojom from '../../../common/mojom'
import { PINNED_VENDOR_KEY } from '../../../common/vendor_icon_map'
import {
  getAvailableModelCapabilities,
  getVendorRailEntries,
  modelHasAllCapabilities,
} from '../../model_utils'
import { CapabilityFilter } from './capability_filter'
import { ModelSearch } from './model_search'
import { VendorRail } from './vendor_rail'
import styles from './style.module.scss'

const MODELS: Mojom.Model[] = [
  {
    key: 'chat-automatic',
    displayName: 'Automatic',
    visionSupport: true,
    audioSupport: false,
    videoSupport: false,
    supportsTools: true,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [],
    isSuggestedModel: true,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'automatic',
        displayMaker: '',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC_AND_PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'chat-claude-sonnet',
    displayName: 'Claude Sonnet',
    visionSupport: true,
    audioSupport: false,
    videoSupport: false,
    supportsTools: true,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [
      Mojom.ModelCapability.SEARCH,
      Mojom.ModelCapability.VISION,
      Mojom.ModelCapability.TOOLS,
    ],
    isSuggestedModel: true,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'claude-sonnet',
        displayMaker: 'Anthropic',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.PREMIUM,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'chat-gpt-mini',
    displayName: 'GPT Mini',
    visionSupport: true,
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [
      Mojom.ModelCapability.FAST,
      Mojom.ModelCapability.VISION,
    ],
    isSuggestedModel: false,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'gpt-mini',
        displayMaker: 'OpenAI',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'chat-llama',
    displayName: 'Llama',
    visionSupport: false,
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [Mojom.ModelCapability.FAST],
    isSuggestedModel: false,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: {
        name: 'llama',
        displayMaker: 'Meta',
        description: '',
        category: Mojom.ModelCategory.CHAT,
        access: Mojom.ModelAccess.BASIC,
        maxAssociatedContentLength: 10000,
        longConversationWarningCharacterLimit: 9700,
      },
      customModelOptions: undefined,
    },
  },
  {
    key: 'custom-local',
    displayName: 'Local Phi',
    visionSupport: false,
    audioSupport: false,
    videoSupport: false,
    supportsTools: false,
    supportedCapabilities: [Mojom.ConversationCapability.CHAT],
    capabilities: [],
    isSuggestedModel: false,
    isNearModel: false,
    supportsPrivateInference: false,
    options: {
      leoModelOptions: undefined,
      customModelOptions: {
        modelRequestName: 'phi3',
        contextSize: 4096,
        apiKey: '',
        modelSystemPrompt: '',
        longConversationWarningCharacterLimit: 3000,
        maxAssociatedContentLength: 4000,
        endpoint: { url: 'http://localhost:11434' },
      },
    },
  },
]

function ModelSelectorChrome() {
  const [selectedVendorKey, setSelectedVendorKey] =
    React.useState(PINNED_VENDOR_KEY)
  const [searchQuery, setSearchQuery] = React.useState('')
  const [capabilityFilters, setCapabilityFilters] = React.useState<
    Mojom.ModelCapability[]
  >([])
  const [filterOpen, setFilterOpen] = React.useState(false)

  const vendorEntries = React.useMemo(
    () => getVendorRailEntries(MODELS),
    [],
  )
  const availableCapabilities = React.useMemo(
    () => getAvailableModelCapabilities(MODELS),
    [],
  )

  const filteredModels = React.useMemo(() => {
    const query = searchQuery.trim().toLowerCase()
    return MODELS.filter((model) => {
      if (
        capabilityFilters.length > 0
        && !modelHasAllCapabilities(model, capabilityFilters)
      ) {
        return false
      }
      if (!query) {
        return true
      }
      const maker = model.options.leoModelOptions?.displayMaker ?? ''
      const requestName = model.options.customModelOptions?.modelRequestName ?? ''
      return (
        model.displayName.toLowerCase().includes(query)
        || maker.toLowerCase().includes(query)
        || requestName.toLowerCase().includes(query)
      )
    })
  }, [searchQuery, capabilityFilters])

  return (
    <div className={styles.menuBody}>
      <VendorRail
        entries={vendorEntries}
        selectedKey={selectedVendorKey}
        onSelect={setSelectedVendorKey}
      />
      <div className={styles.mainPane}>
        <div className={styles.searchAndFilters}>
          <ModelSearch
            value={searchQuery}
            onChange={setSearchQuery}
          />
          <CapabilityFilter
            available={availableCapabilities}
            selected={capabilityFilters}
            onChange={setCapabilityFilters}
            isOpen={filterOpen}
            onOpenChange={setFilterOpen}
          />
        </div>
        <div className={styles.modelList}>
          {filteredModels.length === 0 ? (
            <div className={styles.emptyState}>No models found</div>
          ) : (
            filteredModels.map((model) => (
              <div
                key={model.key}
                style={{
                  padding: '8px 12px',
                  borderRadius: 8,
                  font: 'var(--leo-font-default-regular)',
                }}
              >
                {model.displayName}
                <div
                  style={{
                    font: 'var(--leo-font-small-regular)',
                    color: 'var(--leo-color-text-tertiary)',
                  }}
                >
                  {model.options.leoModelOptions?.displayMaker
                    || model.options.customModelOptions?.modelRequestName
                    || ' '}
                </div>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  )
}

export const _SearchFilterAndVendorRail = {
  render: () => <ModelSelectorChrome />,
}

export default {
  title: 'AI Chat/Model Selector Building Blocks',
  component: ModelSelectorChrome,
} as Meta<typeof ModelSelectorChrome>
