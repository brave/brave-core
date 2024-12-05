/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import * as Mojom from '../../../common/mojom'
import styles from './style.module.scss'
import { getLocale } from '$web-common/locale'
import Button from '@brave/leo/react/button'

interface ActionTypeLabelProps {
  actionType: Mojom.ActionType
  removable?: boolean
  onCloseClick?: () => void
}

function getCategoryAndItem(actionType: Mojom.ActionType): {
  category: string | undefined, item: string | undefined
} {
  switch (actionType) {
      case Mojom.ActionType.SUMMARIZE_SELECTED_TEXT:
        return { category: undefined, item: getLocale('summarizeLabel')}
      case Mojom.ActionType.EXPLAIN:
        return { category: undefined, item: getLocale('explainLabel')}
      case Mojom.ActionType.CREATE_TAGLINE:
        return {
          category: getLocale('createCategoryTitle'),
          item: getLocale('taglineLabel')
        }
      case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
        return {
          category: getLocale('socialMediaPostLabel'),
          item: getLocale('socialMediaShortLabel')
        }
      case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG:
        return {
          category: getLocale('socialMediaPostLabel'),
          item: getLocale('socialMediaLongLabel')
        }
      case Mojom.ActionType.PARAPHRASE:
        return {
          category: getLocale('rewriteCategoryTitle'),
          item: getLocale('paraphraseLabel')
        }
      case Mojom.ActionType.IMPROVE:
        return {
          category: getLocale('rewriteCategoryTitle'),
          item: getLocale('improveLabel')
        }
      case Mojom.ActionType.ACADEMICIZE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('academicizeLabel')
        }
      case Mojom.ActionType.PROFESSIONALIZE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('professionalizeLabel')
        }
      case Mojom.ActionType.PERSUASIVE_TONE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('persuasiveToneLabel')
        }
      case Mojom.ActionType.CASUALIZE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('casualizeLabel')
        }
      case Mojom.ActionType.FUNNY_TONE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('funnyToneLabel')
        }
      case Mojom.ActionType.SHORTEN:
        return {
          category: getLocale('changeLengthLabel'),
          item: getLocale('shortenLabel')
        }
      case Mojom.ActionType.EXPAND:
        return {
          category: getLocale('changeLengthLabel'),
          item: getLocale('expandLabel')
        }
      default:
        return { category: undefined, item: undefined }
  }
}

function ActionTypeLabel (props: ActionTypeLabelProps) {
  const { category, item } = getCategoryAndItem(props.actionType)

  let removeButtonElement = null

  if (props.removable) {
    removeButtonElement = (
      <Button
        className={styles.removeButton}
        fab
        kind="plain-faint"
        title="remove action"
        onClick={props.onCloseClick}
      >
        <Icon name="close"/>
      </Button>
    )
  }

  if (category && item) {
    return (
      <div className={styles.actionTypeLabel}>
        <span className={styles.text}>{category}</span>
        <Icon name='carat-right' />
        <span className={styles.text}>{item}</span>
        {removeButtonElement}
      </div>
    )
  }

  return (
    <div className={styles.actionTypeLabel}>
      <span className={styles.text}>{item}</span>
      {removeButtonElement}
    </div>
  )
}

export default ActionTypeLabel
