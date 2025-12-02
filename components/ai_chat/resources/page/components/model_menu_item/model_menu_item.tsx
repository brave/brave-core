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
import { getModelIcon, NEAR_AI_LEARN_MORE_URL } from '../../../common/constants'
import { NearIcon } from '../near_label/near_label'
import styles from './model_menu_item_style.module.scss'

interface ModelContentProps {
  model: Mojom.Model
  isCurrent: boolean
  showDetails?: boolean
  showPremiumLabel?: boolean
  isDisabled?: boolean
  onClickLearnMore?: () => void
}

const ModelContent = (props: ModelContentProps) => {
  const {
    model,
    isCurrent,
    showDetails,
    showPremiumLabel,
    isDisabled,
    onClickLearnMore,
  } = props

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
    if (model.isNearModel) {
      return (
        <Label
          mode='outline'
          color='neutral'
          className={styles.modelLabel}
        >
          {getLocale(S.CHAT_UI_MODEL_BETA_LABEL)}
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
          [styles.disabled]: isDisabled,
        })}
        data-key={model.key}
      >
        <Icon name={getModelIcon(model.key)} />
      </div>
      <div className={styles.column}>
        <div className={styles.nameAndLabel}>
          <div
            className={classnames({
              [styles.modelName]: true,
              [styles.disabled]: isDisabled,
            })}
          >
            {model.displayName}
            {model.isNearModel && <NearIcon />}
          </div>
          {label}
        </div>
        {showDetails && (
          <>
            <p className={styles.modelSubtitle}>
              {isCustomModel
                ? model.options.customModelOptions?.modelRequestName
                : getLocale(
                    `CHAT_UI_${model.key
                      .toUpperCase()
                      .replaceAll('-', '_')}_SUBTITLE`,
                  )}
            </p>
            {onClickLearnMore && model.isNearModel && (
              <a
                // While we preventDefault, we still need to pass the href
                // here so we can continue to show link previews.
                href={NEAR_AI_LEARN_MORE_URL}
                className={styles.learnMoreLink}
                onClick={(e) => {
                  e.preventDefault()
                  e.stopPropagation()
                  onClickLearnMore()
                }}
              >
                {getLocale(S.CHAT_UI_LEARN_MORE)}
              </a>
            )}
          </>
        )}
      </div>
    </>
  )
}

interface MenuItemProps extends ModelContentProps {
  onClick: () => void
}

export function ModelMenuItem(props: MenuItemProps) {
  const {
    model,
    isCurrent,
    showDetails,
    showPremiumLabel,
    isDisabled,
    onClick,
    onClickLearnMore,
  } = props
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
        isDisabled={isDisabled}
        onClickLearnMore={onClickLearnMore}
      />
    </leo-menu-item>
  )
}

export function ModelOption(props: ModelContentProps) {
  const { model, isCurrent, showDetails, showPremiumLabel, isDisabled } = props

  const content = (
    <ModelContent
      model={model}
      isCurrent={isCurrent}
      showPremiumLabel={showPremiumLabel}
      showDetails={showDetails}
      isDisabled={isDisabled}
    />
  )

  // There is currently no way to disable a menu item in nala dropdowns,
  // so we need to return as a div instead of a leo-option.
  if (isDisabled) {
    return <div className={styles.disabledOption}>{content}</div>
  }

  return <leo-option value={model.key}>{content}</leo-option>
}
