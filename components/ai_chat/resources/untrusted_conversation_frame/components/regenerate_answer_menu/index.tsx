/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import * as Mojom from '../../../common/mojom'
import {
  ModelMenuItem, //
} from '../../../page/components/model_menu_item/model_menu_item'
import { NearLabel } from '../../../page/components/near_label/near_label'
import styles from './style.module.scss'

interface Props {
  isOpen: boolean
  onOpen: () => void
  onClose: () => void
  onRegenerate: (selectedModelKey: string) => void
  leoModels: Mojom.Model[]
  turnModelKey: string
  isNearTeeVerified?: boolean
}

export function RegenerateAnswerMenu(props: Props) {
  const {
    isOpen,
    onOpen,
    onClose,
    onRegenerate,
    leoModels,
    turnModelKey,
    isNearTeeVerified,
  } = props

  const turnModel = leoModels.find((model) => model.key === turnModelKey)
  const modelDisplayName = turnModel?.displayName ?? ''

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
          <div className={styles.iconAndLabel}>
            {turnModel?.isNearModel && isNearTeeVerified && <NearLabel />}
            <Icon
              name='carat-down'
              className={classnames({
                [styles.anchorButtonIcon]: true,
                [styles.anchorButtonIconOpen]: isOpen,
              })}
            />
          </div>
        </div>
      </Button>
      {leoModels.map((model) => (
        <ModelMenuItem
          key={model.key}
          model={model}
          isCurrent={model.key === turnModelKey}
          onClick={() => handleRegenerate(model.key)}
        />
      ))}
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
