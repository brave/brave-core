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
import { ModelMenuItem } from '../model_menu_item/model_menu_item'
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
  const [pinnedKeys, setPinnedKeys] = React.useState<string[]>([
    'chat-claude-sonnet',
    'chat-gpt-mini',
  ])
  const [defaultModelKey, setDefaultModelKey] = React.useState(
    'chat-claude-sonnet',
  )
  const [currentModelKey, setCurrentModelKey] = React.useState(
    'chat-claude-sonnet',
  )
  const [openOptionsKey, setOpenOptionsKey] = React.useState<string | null>(
    null,
  )

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
      const requestName =
        model.options.customModelOptions?.modelRequestName ?? ''
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
            onOpenChange={(open) => {
              setFilterOpen(open)
              if (open) {
                setOpenOptionsKey(null)
              }
            }}
          />
        </div>
        <div className={styles.modelList}>
          {filteredModels.length === 0 ? (
            <div className={styles.emptyState}>No models found</div>
          ) : (
            filteredModels.map((model) => {
              const isPinned = pinnedKeys.includes(model.key)
              return (
                <ModelMenuItem
                  key={model.key}
                  model={model}
                  isCurrent={model.key === currentModelKey}
                  isPinned={isPinned}
                  isDefault={model.key === defaultModelKey}
                  showPremiumLabel={true}
                  showDetails={true}
                  showCapabilitySubtitle={true}
                  isOptionsOpen={openOptionsKey === model.key}
                  onOptionsOpenChange={(open) => {
                    setOpenOptionsKey(open ? model.key : null)
                    if (open) {
                      setFilterOpen(false)
                    }
                  }}
                  onClick={() => setCurrentModelKey(model.key)}
                  onSetAsDefault={() => setDefaultModelKey(model.key)}
                  onTogglePin={
                    model.key === 'chat-automatic'
                      ? undefined
                      : () => {
                          setPinnedKeys((prev) =>
                            prev.includes(model.key)
                              ? prev.filter((key) => key !== model.key)
                              : [model.key, ...prev],
                          )
                        }
                  }
                />
              )
            })
          )}
        </div>
      </div>
    </div>
  )
}

export const _SearchFilterRailAndModelItems = {
  render: () => <ModelSelectorChrome />,
}

export default {
  title: 'AI Chat/Model Selector Building Blocks',
  component: ModelSelectorChrome,
} as Meta<typeof ModelSelectorChrome>
