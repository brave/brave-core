/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import styles from './style.module.scss'

interface Props {
  onClose: () => void
  onRegenerate: (selectedModelKey: string) => void
  leoModels: Mojom.Model[]
  turnModelKey: string
}

export default function RegenerateAnswerMenu(
  props: Props
) {

  const [selectedModel, setSelectedModel] = React.useState(props.turnModelKey)

  const handleModelSelect = (modelKey: string) => {
    setSelectedModel(modelKey)
  }

  const handleSubmit = () => {
    props.onRegenerate(selectedModel)
    props.onClose()
  }

  return (
    <div className={styles.menuContainer}>
      <div className={styles.menuHeader}>
        <div className={styles.menuTitle}>
          {getLocale('regenerateAnswerMenuTitle')}
        </div>
      </div>

      <div className={styles.modelList}>
        {props.leoModels.map((model) => {
          const selected = model.key === selectedModel
          return (
            <leo-menu-item
              key={model.key}
              aria-selected={selected}
              onClick={() => handleModelSelect(model.key)}
            >
              <div className={styles.modelName}>
                {model.displayName}
              </div>
              {selected && (
                <Icon name='check-circle-outline' />
              )}
            </leo-menu-item>
          )
        })}
      </div>
      <div className={styles.menuFooter}>
        <Button
          kind='plain-faint'
          title={getLocale('regenerateAnswerButtonLabel')}
          onClick={handleSubmit}
        >
          <Icon name="refresh" slot='icon-before'/>
          {getLocale('regenerateAnswerButtonLabel')}
        </Button>
      </div>
    </div>
  )
}
