/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import { getModelIcon } from '../../../common/constants'
import styles from './style.module.scss'

interface Props {
  isOpen: boolean
  onOpen: () => void
  onClose: () => void
  onRegenerate: (selectedModelKey: string) => void
  leoModels: Mojom.Model[]
  turnModelKey: string
}

export function RegenerateAnswerMenu(props: Props) {
  const { isOpen, onOpen, onClose, onRegenerate, leoModels, turnModelKey } =
    props

  const modelDisplayName =
    leoModels.find((model) => model.key === turnModelKey)?.displayName ?? ''

  const handleRegenerate = React.useCallback(
    (modelKey: string) => {
      if (!leoModels.some((model) => model.key === modelKey)) {
        return
      }
      onRegenerate(modelKey)
      onClose()
    },
    [onRegenerate, onClose, leoModels],
  )

  return (
    <ButtonMenu
      className={styles.buttonMenu}
      isOpen={isOpen}
      onClose={onClose}
    >
      <div className={styles.menuHeader}>
        {getLocale(S.CHAT_UI_REGENERATE_ANSWER_MENU_TITLE)}
      </div>
      <div className={styles.headerGap} />
      <Button
        slot='anchor-content'
        kind='plain-faint'
        size='tiny'
        onClick={isOpen ? onClose : onOpen}
        title={getLocale(S.CHAT_UI_REGENERATE_ANSWER_MENU_TOOLTIP).replace(
          '$1',
          modelDisplayName,
        )}
        className={classnames({
          [styles.anchorButton]: true,
          [styles.anchorButtonOpen]: isOpen,
        })}
      >
        <div className={styles.anchorButtonContent}>
          <span className={styles.anchorButtonText}>{modelDisplayName}</span>
          <Icon
            name='carat-down'
            className={classnames({
              [styles.anchorButtonIcon]: true,
              [styles.anchorButtonIconOpen]: isOpen,
            })}
          />
        </div>
      </Button>
      {leoModels.map((model) => {
        const selected = model.key === turnModelKey
        return (
          <leo-menu-item
            key={model.key}
            data-key={model.key}
            onClick={() => handleRegenerate(model.key)}
            aria-selected={selected || null}
          >
            <div className={styles.modelIconAndName}>
              <Icon name={getModelIcon(model.key)} />
              {model.displayName}
            </div>
            {selected && (
              <Label
                mode='loud'
                color='primary'
              >
                {getLocale(S.CHAT_UI_CURRENT_LABEL)}
              </Label>
            )}
          </leo-menu-item>
        )
      })}
      <div className={styles.footerGap} />
      <div className={styles.menuFooter}>
        <leo-menu-item
          onClick={() => handleRegenerate(turnModelKey)}
          data-key='retrySameModel'
        >
          <Icon name='refresh' />
          {getLocale(S.CHAT_UI_RETRY_SAME_MODEL_BUTTON_LABEL)}
        </leo-menu-item>
      </div>
    </ButtonMenu>
  )
}
