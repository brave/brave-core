// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'
import useIsConversationVisible from '../../hooks/useIsConversationVisible'
import { camelCase } from '$web-common/camelCase'

export interface Props {
  setIsConversationsListOpen?: (value: boolean) => unknown
}

export default function FeatureMenu(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const handleSettingsClick = () => {
    aiChatContext.uiHandler?.openAIChatSettings()
  }

  // If conversation is in the conversations list, then it has been committed
  // as a conversation with content.
  const isActiveConversationPermanent = useIsConversationVisible(conversationContext.conversationUuid)

  const customModels = conversationContext.allModels.filter(
    (model) => model.options.customModelOptions
  )
  const leoModels = conversationContext.allModels.filter(
    (model) => model.options.leoModelOptions
  )

  return (
    <ButtonMenu className={styles.buttonMenu}>
      <Button
        slot='anchor-content'
        title={getLocale(StringIds.LeoSettingsTooltipLabel)}
        fab
        kind='plain-faint'
      >
        <Icon name='more-vertical' />
      </Button>
      <div className={styles.menuSectionTitle}>
        {getLocale(StringIds.MenuTitleModels)}
      </div>
      {leoModels.map((model) => {
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={
              model.key === conversationContext.currentModel?.key || null
            }
            onClick={() => conversationContext.setCurrentModel(model)}
          >
            <div className={styles.menuItemWithIcon}>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {getLocale(`${camelCase(model.key)}Subtitle`)}
                </p>
              </div>
              {model.options.leoModelOptions?.access ===
                Mojom.ModelAccess.PREMIUM &&
                !aiChatContext.isPremiumUser && (
                  <Label
                    className={styles.modelLabel}
                    mode={'outline'}
                    color='blue'
                  >
                    {getLocale(StringIds.ModelPremiumLabelNonPremium)}
                  </Label>
                )}
            </div>
          </leo-menu-item>
        )
      })}
      {customModels.length > 0 && (
        <>
          <div className={styles.menuSeparator} />
          <div className={styles.menuSectionCustomModel}>
            {getLocale(StringIds.MenuTitleCustomModels)}
          </div>
        </>
      )}
      {customModels.map((model) => {
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={
              model.key === conversationContext.currentModel?.key || null
            }
            onClick={() => conversationContext.setCurrentModel(model)}
          >
            <div className={styles.menuItemWithIcon}>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {model.options.customModelOptions?.modelRequestName}
                </p>
              </div>
            </div>
          </leo-menu-item>
        )
      })}
      <div className={styles.menuSeparator} />

      {aiChatContext.isStandalone && isActiveConversationPermanent && <>
        <leo-menu-item onClick={() => aiChatContext.setEditingConversationId(conversationContext.conversationUuid!)}>
          <div className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}>
            <Icon name='edit-pencil' />
            <div className={styles.menuText}>
              <div>{getLocale(StringIds.MenuRenameConversation)}</div>
            </div>
          </div>
        </leo-menu-item>
        <leo-menu-item onClick={() => aiChatContext.setDeletingConversationId(conversationContext.conversationUuid!)}>
          <div className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}>
            <Icon name='trash' />
            <div className={styles.menuText}>
              <div>{getLocale(StringIds.MenuDeleteConversation)}</div>
            </div>
          </div>
        </leo-menu-item>
        <div className={styles.menuSeparator} />
      </>}

      {!aiChatContext.isPremiumUser && (
        <leo-menu-item onClick={aiChatContext.goPremium}>
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem
            )}
          >
            <Icon name='lock-open' />
            <span className={styles.menuText}>
              {getLocale(StringIds.MenuGoPremium)}
            </span>
          </div>
        </leo-menu-item>
      )}

      {aiChatContext.isPremiumUser && (
        <leo-menu-item onClick={aiChatContext.managePremium}>
          <div
            className={classnames(
              styles.menuItemWithIcon,
              styles.menuItemMainItem
            )}
          >
            <Icon name='lock-open' />
            <span className={styles.menuText}>
              {getLocale(StringIds.MenuManageSubscription)}
            </span>
          </div>
        </leo-menu-item>
      )}
      {!aiChatContext.isStandalone && aiChatContext.isHistoryFeatureEnabled && (
        <>
          <leo-menu-item
            onClick={() => props.setIsConversationsListOpen?.(true)}
          >
            <div
              className={classnames(
                styles.menuItemWithIcon,
                styles.menuItemMainItem
              )}
            >
              <Icon name='history' />
              <span className={styles.menuText}>{getLocale(StringIds.MenuConversationHistory)}</span>
            </div>
          </leo-menu-item>
        </>
      )}
      <leo-menu-item onClick={handleSettingsClick}>
        <div
          className={classnames(
            styles.menuItemWithIcon,
            styles.menuItemMainItem
          )}
        >
          <Icon name='settings' />
          <span className={styles.menuText}>{getLocale(StringIds.MenuSettings)}</span>
        </div>
      </leo-menu-item>
    </ButtonMenu>
  )
}
