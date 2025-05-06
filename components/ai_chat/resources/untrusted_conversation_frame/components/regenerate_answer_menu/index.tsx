/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import styles from './style.module.scss'

interface Props {
  isOpen: boolean
  onOpen: () => void
  onClose: () => void
  onRegenerate: (selectedModelKey: string) => void
  leoModels: Mojom.Model[]
  turnModelKey: string
}

export default function RegenerateAnswerMenu(props: Props) {
  const { isOpen, onOpen, onClose, onRegenerate, leoModels, turnModelKey } =
    props
  const [selectedModel, setSelectedModel] = React.useState(turnModelKey)

  const handleModelSelect = (modelKey: string) => {
    setSelectedModel(modelKey)
  }

  const handleSubmit = () => {
    onRegenerate(selectedModel)
    onClose()
  }

  return (
    <ButtonMenu
      className={styles.buttonMenu}
      isOpen={isOpen}
      onClose={onClose}
    >
      <div className={styles.menuHeader}>
        {getLocale('regenerateAnswerMenuTitle')}
      </div>
      <div className={styles.headerGap} />
      <Button
        fab
        slot='anchor-content'
        size='small'
        kind='plain-faint'
        onClick={onOpen}
        className={styles.anchorButton}
      >
        <Icon name='refresh' />
      </Button>
      {leoModels.map((model) => {
        const selected = model.key === selectedModel
        return (
          <leo-menu-item
            key={model.key}
            onClick={() => {
              onOpen()
              handleModelSelect(model.key)
            }}
            aria-selected={selected || null}
          >
            {model.displayName}
            {selected && <Icon name='check-circle-outline' />}
          </leo-menu-item>
        )
      })}
      <div className={styles.footerGap} />
      <div className={styles.menuFooter}>
        <Button
          kind='plain-faint'
          title={getLocale('regenerateAnswerButtonLabel')}
          onClick={handleSubmit}
        >
          <div className={styles.regenerateButtonContent}>
            <Icon name='refresh' />
            {getLocale('regenerateAnswerButtonLabel')}
          </div>
        </Button>
      </div>
    </ButtonMenu>
  )
}
