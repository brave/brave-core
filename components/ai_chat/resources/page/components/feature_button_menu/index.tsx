// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import { getLocale } from '$web-common/locale'
import getPageHandlerInstance, * as mojom from '../../api/page_handler'
import DataContext from '../../state/context'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'

export default function FeatureMenu() {
  const context = React.useContext(DataContext)

  const handleSettingsClick = () => {
    getPageHandlerInstance().pageHandler.openBraveLeoSettings()
  }

  const handleNewConversationClick = () => {
    getPageHandlerInstance().pageHandler.clearConversationHistory()
  }

  return (
    <ButtonMenu>
      <Button
        className={styles.settingsButton}
        slot='anchor-content'
        title={getLocale('leoSettingsTooltipLabel')}
        fab
        kind="plain-faint"
      >
        <Icon name='settings' />
      </Button>
      <div className={styles.menuSectionTitle}>
        {getLocale('menuTitleModels')}
      </div>
      <div className={styles.menuSubtitle}>
        <Icon name='message-bubble-comments' />
        <span>{getLocale('modelCategory-chat')}</span>
      </div>

      {context.allModels.map((model) => (
        <leo-menu-item
          key={model.key}
          aria-selected={model.key === context.currentModel?.key || undefined}
          onClick={() => context.setCurrentModel(model)}
        >
          <div className={styles.menuItemWithIcon}>
            <div className={styles.menuText}>
              <div>{model.displayName}</div>
              <p className={styles.modelSubtitle}>
                {getLocale(`braveLeoModelSubtitle-${model.key}`)}
              </p>
            </div>
            {model.access === mojom.ModelAccess.PREMIUM && (
              <Icon
                className={classnames({
                  [styles.lockOpen]: context.isPremiumUser
                })}
                name={context.isPremiumUser ? 'lock-open' : 'lock-plain'}
              />
            )}
            {model.access === mojom.ModelAccess.BASIC_AND_PREMIUM && (
              <Label
               className={styles.modelFreemiumLabel}
               mode={context.isPremiumUser ? 'loud' : 'default'}
               color='blue'
              >
                {context.isPremiumUser ? getLocale('modelFreemiumLabelPremium') : getLocale('modelFreemiumLabelNonPremium')}
              </Label>
            )}
          </div>
        </leo-menu-item>
      ))}

      <div className={styles.menuSeparator} />

      <leo-menu-item onClick={handleNewConversationClick}>
        <div className={classnames(styles.menuItemWithIcon, styles.menuItemMainItem)}>
          <Icon name='erase' />
          <span className={styles.menuText}>{getLocale('menuNewChat')}</span>
        </div>
      </leo-menu-item>

      {!context.isPremiumUser &&
      <leo-menu-item onClick={context.goPremium}>
        <div className={classnames(styles.menuItemWithIcon, styles.menuItemMainItem)}>
          <Icon name='lock-open' />
          <span className={styles.menuText}>{getLocale('menuGoPremium')}</span>
        </div>
      </leo-menu-item>
      }

      {context.isPremiumUser &&
      <leo-menu-item onClick={context.managePremium}>
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
