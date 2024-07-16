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
import * as mojom from '../../api'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

export default function FeatureMenu() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const handleSettingsClick = () => {
    aiChatContext.uiHandler?.openAIChatSettings()
  }

  const customModels = conversationContext.allModels.filter(model => model.options.customModelOptions !== undefined)
  const leoModels = conversationContext.allModels.filter(model => model.options.leoModelOptions !== undefined)

  return (
    <ButtonMenu
      className={styles.buttonMenu}
    >
      <Button
        slot='anchor-content'
        title={getLocale('leoSettingsTooltipLabel')}
        fab
        kind="plain-faint"
      >
        <Icon name='more-vertical' />
      </Button>
      <div className={styles.menuSectionTitle}>
        {getLocale('menuTitleModels')}
      </div>
      {leoModels.map((model) => {
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={model.key === conversationContext.currentModel?.key || null}
            onClick={() => conversationContext.setCurrentModel(model)}
          >
            <div className={styles.menuItemWithIcon}>
              <div className={styles.menuText}>
                <div>{model.displayName}</div>
                <p className={styles.modelSubtitle}>
                  {getLocale(`braveLeoModelSubtitle-${model.key}`)}
                </p>
              </div>
              {model.options.leoModelOptions?.access ===
                mojom.ModelAccess.PREMIUM &&
                !aiChatContext.isPremiumUser && (
                  <Label
                    className={styles.modelLabel}
                    mode={'outline'}
                    color='blue'
                  >
                    {getLocale('modelPremiumLabelNonPremium')}
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
            {getLocale('menuTitleCustomModels')}
          </div>
        </>
      )}
      {customModels.map((model) => {
        return (
          <leo-menu-item
            key={model.key}
            aria-selected={model.key === conversationContext.currentModel?.key || null}
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

      {!aiChatContext.isPremiumUser &&
      <leo-menu-item onClick={aiChatContext.goPremium}>
        <div className={classnames(styles.menuItemWithIcon, styles.menuItemMainItem)}>
          <Icon name='lock-open' />
          <span className={styles.menuText}>{getLocale('menuGoPremium')}</span>
        </div>
      </leo-menu-item>
      }

      {aiChatContext.isPremiumUser &&
      <leo-menu-item onClick={aiChatContext.managePremium}>
        <div className={classnames(styles.menuItemWithIcon, styles.menuItemMainItem)}>
          <Icon name='lock-open' />
          <span className={styles.menuText}>{getLocale('menuManageSubscription')}</span>
        </div>
      </leo-menu-item>
      }

      <leo-menu-item onClick={handleSettingsClick}>
        <div className={classnames(styles.menuItemWithIcon, styles.menuItemMainItem)}>
          <Icon name='settings' />
          <span className={styles.menuText}>{getLocale('menuSettings')}</span>
        </div>
      </leo-menu-item>
    </ButtonMenu>
  )
}
