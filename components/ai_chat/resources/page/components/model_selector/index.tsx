/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import { getLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import { getModelIcon } from '../../../common/constants'
import styles from './style.module.scss'

const AUTO_MODEL_KEY = 'chat-automatic'

export function ModelSelector() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  // State
  const [isOpen, setIsOpen] = React.useState(false)
  const [showAllModels, setShowAllModels] = React.useState(false)

  // Memos
  const suggestedModels = React.useMemo(() => {
    return conversationContext.allModels.filter(
      (model) => model.isSuggestedModel,
    )
  }, [conversationContext.allModels])

  const models = React.useMemo(() => {
    // Show all BASIC_AND_PREMIUM models if showAllModels is true
    if (showAllModels) {
      return conversationContext.allModels.filter(
        (model) =>
          model.options.leoModelOptions?.access
          === Mojom.ModelAccess.BASIC_AND_PREMIUM,
      )
    }

    // Find the Auto model (chat-automatic)
    const autoModel = conversationContext.allModels.find(
      (model) => model.key === AUTO_MODEL_KEY,
    )
    const defaultModel = conversationContext.userDefaultModel
    const currentModel = conversationContext.currentModel
    const recommendedList: Mojom.Model[] = []

    // Keep Auto model first in list
    if (autoModel) {
      recommendedList.push(autoModel)
    }

    // Add defaultModel if it exists and is not Auto
    if (defaultModel && defaultModel.key !== AUTO_MODEL_KEY) {
      recommendedList.push(defaultModel)
    }

    // Add currentModel if it exists and is not Auto or defaultModel
    if (
      currentModel
      && currentModel.key !== AUTO_MODEL_KEY
      && currentModel.key !== defaultModel?.key
    ) {
      recommendedList.push(currentModel)
    }

    // Add suggestedModels that are not already in the list
    const existingKeys = new Set(recommendedList.map((model) => model.key))
    const filteredSuggestedModels = suggestedModels.filter(
      (model) => !existingKeys.has(model.key),
    )
    recommendedList.push(...filteredSuggestedModels)

    return recommendedList
  }, [
    showAllModels,
    conversationContext.allModels,
    conversationContext.userDefaultModel,
    suggestedModels,
  ])

  const premiumModels = React.useMemo(
    () =>
      conversationContext.allModels.filter(
        (model) =>
          model.options.leoModelOptions?.access === Mojom.ModelAccess.PREMIUM,
      ),
    [conversationContext.allModels],
  )

  const customModels = React.useMemo(
    () =>
      conversationContext.allModels.filter(
        (model) => model.options.customModelOptions,
      ),
    [conversationContext.allModels],
  )

  return (
    <ButtonMenu
      className={styles.buttonMenu}
      isOpen={isOpen}
      onClose={() => setIsOpen(false)}
      placement='top-end'
      flip={false}
    >
      <Button
        slot='anchor-content'
        kind='plain-faint'
        size='tiny'
        onClick={() => setIsOpen(!isOpen)}
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
          <Icon
            name='carat-down'
            slot='icon-after'
            className={classnames({
              [styles.anchorButtonIcon]: true,
              [styles.anchorButtonIconOpen]: isOpen,
            })}
          />
        </div>
      </Button>

      {models.map((model) => {
        const isCustomModel = model.options.customModelOptions
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={
              model.key === conversationContext.currentModel?.key || null
            }
            onClick={() => conversationContext.setCurrentModel(model)}
          >
            <div
              className={styles.modelIcon}
              data-key={model.key}
            >
              <Icon name={getModelIcon(model.key)} />
            </div>
            <div className={styles.menuText}>
              <div>{model.displayName}</div>
              <p className={styles.modelSubtitle}>
                {isCustomModel
                  ? model.options.customModelOptions?.modelRequestName
                  : getLocale(
                      `CHAT_UI_${model.key
                        .toUpperCase()
                        .replaceAll('-', '_')}_SUBTITLE`,
                    )}
              </p>
            </div>
            {model.options.leoModelOptions?.access === Mojom.ModelAccess.PREMIUM
              && !aiChatContext.isPremiumUser && (
                <Label
                  className={styles.modelLabel}
                  mode={'outline'}
                  color='blue'
                >
                  {getLocale(S.CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM)}
                </Label>
              )}
            {isCustomModel && (
              <Label
                className={styles.modelLabel}
                mode='default'
                color='blue'
              >
                {getLocale(S.CHAT_UI_MODEL_LOCAL_LABEL)}
              </Label>
            )}
          </leo-menu-item>
        )
      })}

      {showAllModels
        && premiumModels.map((model) => {
          return (
            <leo-menu-item
              key={model.key}
              aria-selected={
                model.key === conversationContext.currentModel?.key || null
              }
              onClick={() => conversationContext.setCurrentModel(model)}
            >
              <div
                className={styles.modelIcon}
                data-key={model.key}
              >
                <Icon name={getModelIcon(model.key)} />
              </div>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {getLocale(
                    `CHAT_UI_${model.key
                      .toUpperCase()
                      .replaceAll('-', '_')}_SUBTITLE`,
                  )}
                </p>
              </div>
              {!aiChatContext.isPremiumUser && (
                <Label
                  className={styles.modelLabel}
                  mode={'outline'}
                  color='blue'
                >
                  {getLocale(S.CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM)}
                </Label>
              )}
            </leo-menu-item>
          )
        })}

      {showAllModels
        && customModels.map((model) => {
          return (
            <leo-menu-item
              key={model.key}
              aria-selected={
                model.key === conversationContext.currentModel?.key || null
              }
              onClick={() => conversationContext.setCurrentModel(model)}
            >
              <div
                className={styles.modelIcon}
                data-key={model.key}
              >
                <Icon name={getModelIcon(model.key)} />
              </div>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {model.options.customModelOptions?.modelRequestName}
                </p>
              </div>
              <Label
                className={styles.modelLabel}
                mode='default'
                color='blue'
              >
                {getLocale(S.CHAT_UI_MODEL_LOCAL_LABEL)}
              </Label>
            </leo-menu-item>
          )
        })}

      <div className={styles.footerGap} />
      <div
        className={classnames({
          [styles.menuFooter]: true,
          [styles.menuFooterExtraPadding]: !showAllModels,
        })}
      >
        <leo-menu-item
          data-testid='show-all-models-button'
          onClick={(e) => {
            e.stopPropagation()
            setShowAllModels(!showAllModels)
          }}
        >
          <div className={styles.menuFooterIconAndText}>
            {showAllModels && <Icon name='carat-left' />}
            {getLocale(
              showAllModels
                ? S.CHAT_UI_RECOMMENDED_MODELS_BUTTON
                : S.CHAT_UI_SHOW_ALL_MODELS_BUTTON,
            )}
          </div>
          {!showAllModels && <Icon name='carat-right' />}
        </leo-menu-item>
      </div>
    </ButtonMenu>
  )
}
