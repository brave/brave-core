/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import * as mojom from '../../api/page_handler'
import styles from './style.module.scss'
import { getLocale } from '$web-common/locale'

interface ActionTypeLabelProps {
  actionType: mojom.ActionType
}

function getCategoryAndItem(actionType: mojom.ActionType): {
  category: string | undefined, item: string | undefined
} {
  switch (actionType) {
      case mojom.ActionType.SUMMARIZE_SELECTED_TEXT:
        return { category: undefined, item: getLocale('summarizeLabel')}
      case mojom.ActionType.EXPLAIN:
        return { category: undefined, item: getLocale('explainLabel')}
      case mojom.ActionType.CREATE_TAGLINE:
        return {
          category: getLocale('createCategoryTitle'),
          item: getLocale('taglineLabel')
        }
      case mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
        return {
          category: getLocale('socialMediaPostLabel'),
          item: getLocale('socialMediaShortLabel')
        }
      case mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG:
        return {
          category: getLocale('socialMediaPostLabel'),
          item: getLocale('socialMediaLongLabel')
        }
      case mojom.ActionType.PARAPHRASE:
        return {
          category: getLocale('rewriteCategoryTitle'),
          item: getLocale('paraphraseLabel')
        }
      case mojom.ActionType.IMPROVE:
        return {
          category: getLocale('rewriteCategoryTitle'),
          item: getLocale('improveLabel')
        }
      case mojom.ActionType.ACADEMICIZE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('academicizeLabel')
        }
      case mojom.ActionType.PROFESSIONALIZE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('professionalizeLabel')
        }
      case mojom.ActionType.PERSUASIVE_TONE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('persuasiveToneLabel')
        }
      case mojom.ActionType.CASUALIZE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('casualizeLabel')
        }
      case mojom.ActionType.FUNNY_TONE:
        return {
          category: getLocale('changeToneLabel'),
          item: getLocale('funnyToneLabel')
        }
      case mojom.ActionType.SHORTEN:
        return {
          category: getLocale('changeLengthLabel'),
          item: getLocale('shortenLabel')
        }
      case mojom.ActionType.EXPAND:
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

  if (category && item) {
    return (
      <div className={styles.actionTypeLabel}>
        <span className={styles.text}>{category}</span>
        <Icon name='carat-right' />
        <span className={styles.text}>{item}</span>
      </div>
    )
  }

  return (
    <div className={styles.actionTypeLabel}>
      <span className={styles.text}>{item}</span>
    </div>
  )
}

export default ActionTypeLabel
