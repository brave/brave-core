/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import { getLocale } from '$web-common/locale'
import {
  AUTOMATIC_MODEL_KEY,
  NEAR_AI_LEARN_MORE_URL,
} from '../../../common/constants'
import {
  LOCAL_VENDOR_KEY,
  PINNED_VENDOR_KEY,
} from '../../../common/vendor_icon_map'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import {
  getAvailableModelCapabilities,
  getVendorRailEntries,
  isSelectableModel,
  modelHasAllCapabilities,
} from '../../model_utils'
import { ModelMenuItem } from '../model_menu_item/model_menu_item'
import { NearIcon } from '../near_label/near_label'
import { FuzzyFinder } from '../filter_menu/fuzzy_finder'
import { matches } from '../filter_menu/query'
import { CapabilityFilter } from './capability_filter'
import { ModelSearch } from './model_search'
import { VendorRail } from './vendor_rail'
import styles from './style.module.scss'

function matchesModelQuery(query: string, model: Mojom.Model) {
  if (!query) {
    return true
  }
  const finder = new FuzzyFinder(query)
  const nameMatch = matches(finder, model.displayName)
  const maker = model.options.leoModelOptions?.displayMaker ?? ''
  const makerMatch = maker ? matches(finder, maker) : undefined
  const requestName = model.options.customModelOptions?.modelRequestName ?? ''
  const requestMatch = requestName ? matches(finder, requestName) : undefined
  return !!(nameMatch || makerMatch || requestMatch)
}

export function ModelSelector() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const [isOpen, setIsOpen] = React.useState(false)
  const [selectedVendorKey, setSelectedVendorKey] =
    React.useState(PINNED_VENDOR_KEY)
  const [searchQuery, setSearchQuery] = React.useState('')
  const [capabilityFilters, setCapabilityFilters] = React.useState<
    Mojom.ModelCapability[]
  >([])
  // Only one nested popover (filter or a model's options) can be open.
  const [openPopover, setOpenPopover] = React.useState<
    null | { kind: 'filter' } | { kind: 'model-options'; modelKey: string }
  >(null)

  const selectableModels = React.useMemo(
    () => conversationContext.allModels.filter(isSelectableModel),
    [conversationContext.allModels],
  )

  const availableCapabilities = React.useMemo(
    () => getAvailableModelCapabilities(selectableModels),
    [selectableModels],
  )

  // Drop selections for capabilities that no longer appear on any model.
  React.useEffect(() => {
    setCapabilityFilters((prev) => {
      const next = prev.filter((capability) =>
        availableCapabilities.includes(capability),
      )
      return next.length === prev.length ? prev : next
    })
  }, [availableCapabilities])

  const pinnedModelKeys = React.useMemo(() => {
    const keys = aiChatContext.pinnedModelKeys ?? []
    if (keys.length > 0) {
      return keys
    }

    // Empty pref: seed with the same models that used to appear in the
    // recommended short list (excluding Auto, which is always shown first).
    const defaultKeys: string[] = []
    const defaultModel = conversationContext.userDefaultModel
    const currentModel = conversationContext.currentModel

    if (
      defaultModel
      && defaultModel.key !== AUTOMATIC_MODEL_KEY
      && isSelectableModel(defaultModel)
    ) {
      defaultKeys.push(defaultModel.key)
    }

    if (
      currentModel
      && currentModel.key !== AUTOMATIC_MODEL_KEY
      && currentModel.key !== defaultModel?.key
      && isSelectableModel(currentModel)
    ) {
      defaultKeys.push(currentModel.key)
    }

    const existing = new Set(defaultKeys)
    for (const model of selectableModels) {
      if (
        model.isSuggestedModel
        && model.key !== AUTOMATIC_MODEL_KEY
        && !existing.has(model.key)
      ) {
        defaultKeys.push(model.key)
      }
    }

    return defaultKeys
  }, [
    aiChatContext.pinnedModelKeys,
    selectableModels,
    conversationContext.userDefaultModel,
    conversationContext.currentModel,
  ])

  const pinnedKeySet = React.useMemo(
    () => new Set(pinnedModelKeys),
    [pinnedModelKeys],
  )

  const vendorEntries = React.useMemo(
    () => getVendorRailEntries(selectableModels),
    [selectableModels],
  )

  const models = React.useMemo(() => {
    const query = searchQuery.trim()

    // Search is global across all models; vendor rail only scopes the list
    // when the search box is empty.
    let list: Mojom.Model[] = []
    if (query) {
      list = selectableModels.slice()
    } else if (selectedVendorKey === PINNED_VENDOR_KEY) {
      const autoModel = selectableModels.find(
        (model) => model.key === AUTOMATIC_MODEL_KEY,
      )
      if (autoModel) {
        list.push(autoModel)
      }
      for (const key of pinnedModelKeys) {
        if (key === AUTOMATIC_MODEL_KEY) {
          continue
        }
        const model = selectableModels.find((m) => m.key === key)
        if (model) {
          list.push(model)
        }
      }
    } else if (selectedVendorKey === LOCAL_VENDOR_KEY) {
      list = selectableModels.filter(
        (model) => !!model.options.customModelOptions,
      )
    } else {
      list = selectableModels.filter(
        (model) =>
          model.options.leoModelOptions?.displayMaker === selectedVendorKey,
      )
    }

    if (capabilityFilters.length > 0) {
      list = list.filter((model) =>
        modelHasAllCapabilities(model, capabilityFilters),
      )
    }

    if (query) {
      list = list.filter((model) => matchesModelQuery(query, model))
    }

    return list
  }, [
    selectedVendorKey,
    selectableModels,
    pinnedModelKeys,
    capabilityFilters,
    searchQuery,
  ])

  const onClickLearnMore = React.useCallback(() => {
    aiChatContext.api.uiHandler?.openURL({
      url: NEAR_AI_LEARN_MORE_URL,
    })
  }, [aiChatContext.api.uiHandler])

  const handleClose = React.useCallback(() => {
    setIsOpen(false)
    setSearchQuery('')
    setCapabilityFilters([])
    setSelectedVendorKey(PINNED_VENDOR_KEY)
    setOpenPopover(null)
  }, [])

  const emptyMessage = React.useMemo(() => {
    if (selectedVendorKey === LOCAL_VENDOR_KEY && !searchQuery) {
      return getLocale(S.CHAT_UI_LOCAL_MODELS_EMPTY)
    }
    return getLocale(S.CHAT_UI_NO_MODELS_FOUND)
  }, [selectedVendorKey, searchQuery])

  return (
    <ButtonMenu
      className={styles.buttonMenu}
      isOpen={isOpen}
      onChange={(e) => {
        if (e.isOpen) {
          setIsOpen(true)
        } else {
          handleClose()
        }
      }}
      positionStrategy='fixed'
    >
      <Button
        slot='anchor-content'
        kind='plain-faint'
        size='tiny'
        className={classnames({
          [styles.anchorButton]: true,
          [styles.anchorButtonOpen]: isOpen,
        })}
        data-testid='anchor-button'
      >
        <div className={styles.anchorButtonContent}>
          <span className={styles.anchorButtonText}>
            {conversationContext.currentModel?.displayName ?? ''}
          </span>
          <div className={styles.icons}>
            {conversationContext.currentModel?.isNearModel && <NearIcon />}
            <Icon
              name='carat-down'
              slot='icon-after'
              className={classnames({
                [styles.anchorButtonIcon]: true,
                [styles.anchorButtonIconOpen]: isOpen,
              })}
            />
          </div>
        </div>
      </Button>

      {/* TODO: This should be based off of conversationCapability
      in the future. https://github.com/brave/brave-browser/issues/49261 */}
      {aiChatContext.isAIChatAgentProfile
        && aiChatContext.isAIChatAgentProfileFeatureEnabled && (
          <Alert
            type='info'
            className={styles.alert}
          >
            <div className={styles.alertText}>
              {getLocale(S.CHAT_UI_AGENT_MODE_MODEL_INFO)}
            </div>
          </Alert>
        )}

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
              isOpen={openPopover?.kind === 'filter'}
              onOpenChange={(open) =>
                setOpenPopover(open ? { kind: 'filter' } : null)
              }
            />
          </div>
          <div className={styles.modelList}>
            {models.length === 0 ? (
              <div
                className={styles.emptyState}
                data-testid='model-selector-empty'
              >
                {emptyMessage}
              </div>
            ) : (
              models.map((model) => {
                const isPinned = pinnedKeySet.has(model.key)
                const isDefault =
                  model.key === conversationContext.userDefaultModel?.key
                return (
                  <ModelMenuItem
                    key={model.key}
                    model={model}
                    isCurrent={
                      model.key === conversationContext.currentModel?.key
                    }
                    isPinned={isPinned}
                    isDefault={isDefault}
                    showPremiumLabel={!aiChatContext.isPremiumUser}
                    showDetails={true}
                    showCapabilitySubtitle={true}
                    isMobile={aiChatContext.isMobile}
                    isOptionsOpen={
                      openPopover?.kind === 'model-options'
                      && openPopover.modelKey === model.key
                    }
                    onOptionsOpenChange={(open) =>
                      setOpenPopover(
                        open
                          ? { kind: 'model-options', modelKey: model.key }
                          : null,
                      )
                    }
                    onClick={() => {
                      conversationContext.setCurrentModel(model)
                      handleClose()
                    }}
                    onClickLearnMore={onClickLearnMore}
                    onSetAsDefault={() =>
                      aiChatContext.setDefaultModelKey(model.key)
                    }
                    onTogglePin={
                      // Automatic is always in the pinned list and cannot be
                      // pinned/unpinned by the user.
                      model.key === AUTOMATIC_MODEL_KEY
                        ? undefined
                        : () => {
                            const persisted =
                              aiChatContext.pinnedModelKeys ?? []
                            if (persisted.length === 0) {
                              // Pref is empty and the UI is using the
                              // recommended-list seed — materialize that set
                              // into the pref before/while applying this pin
                              // change so other defaults aren't lost.
                              const next = new Set(pinnedModelKeys)
                              if (isPinned) {
                                next.delete(model.key)
                              } else {
                                next.add(model.key)
                              }
                              for (const key of next) {
                                aiChatContext.setModelPinned(key, true)
                              }
                              return
                            }

                            aiChatContext.setModelPinned(
                              model.key,
                              !isPinned,
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
    </ButtonMenu>
  )
}
