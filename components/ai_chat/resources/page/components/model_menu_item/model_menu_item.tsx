/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { getModelIcon } from '../../../common/constants'
import styles from './model_menu_item_style.module.scss'

interface ModelContentProps {
  model: Mojom.Model
  isCurrent: boolean
  showDetails?: boolean
  showPremiumLabel?: boolean
}

const ModelContent = (props: ModelContentProps) => {
  const { model, isCurrent, showDetails, showPremiumLabel } = props

  const isCustomModel = model.options.customModelOptions

  const label = React.useMemo(() => {
    if (isCurrent) {
      return (
        <Label
          mode='loud'
          color='primary'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_CURRENT_LABEL)}
        </Label>
      )
    }
    if (
      model.options.leoModelOptions?.access === Mojom.ModelAccess.PREMIUM
      && showPremiumLabel
    ) {
      return (
        <Label
          mode='outline'
          color='blue'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM)}
        </Label>
      )
    }
    if (isCustomModel) {
      return (
        <Label
          mode='default'
          color='blue'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_MODEL_LOCAL_LABEL)}
        </Label>
      )
    }
    return null
  }, [isCurrent, showPremiumLabel, isCustomModel, model])

  return (
    <>
      <div
        className={classnames({
          [styles.modelIcon]: true,
          [styles.modelIconDetailed]: showDetails,
        })}
        data-key={model.key}
      >
        <Icon name={getModelIcon(model.key)} />
      </div>
      <div className={styles.column}>
        <div className={styles.nameAndLabel}>
          <div className={styles.modelName}>{model.displayName}</div>
          {label}
        </div>
        {showDetails && (
          <p className={styles.modelSubtitle}>
            {isCustomModel
              ? model.options.customModelOptions?.modelRequestName
              : getLocale(
                  `CHAT_UI_${model.key
                    .toUpperCase()
                    .replaceAll('-', '_')}_SUBTITLE`,
                )}
          </p>
        )}
      </div>
    </>
  )
}

interface MenuItemProps extends ModelContentProps {
  onClick: () => void
}

export function ModelMenuItem(props: MenuItemProps) {
  const { model, isCurrent, showDetails, showPremiumLabel, onClick } = props
  return (
    <leo-menu-item
      data-key={model.key}
      aria-selected={isCurrent || null}
      onClick={onClick}
    >
      <ModelContent
        model={model}
        isCurrent={isCurrent}
        showPremiumLabel={showPremiumLabel}
        showDetails={showDetails}
      />
    </leo-menu-item>
  )
}

export function ModelOption(props: ModelContentProps) {
  const { model, isCurrent, showDetails, showPremiumLabel } = props
  return (
    <leo-option value={model.key}>
      <ModelContent
        model={model}
        isCurrent={isCurrent}
        showPremiumLabel={showPremiumLabel}
        showDetails={showDetails}
      />
    </leo-option>
  )
}
