/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import * as React from 'react'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../mojom'
import styles from './style.module.scss'

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
        return { category: undefined, item: getLocale(StringIds.ContextSummarizeText)}
      case Mojom.ActionType.EXPLAIN:
        return { category: undefined, item: getLocale(StringIds.ContextExplain)}
      case Mojom.ActionType.CREATE_TAGLINE:
        return {
          category: getLocale(StringIds.ContextCreateTagline),
          item: getLocale(StringIds.ContextCreateTagline)
        }
      case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
        return {
          category: getLocale(StringIds.ContextCreateSocialMediaPost),
          item: getLocale(StringIds.ContextCreateSocialMediaCommentShort)
        }
      case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG:
        return {
          category: getLocale(StringIds.ContextCreateSocialMediaPost),
          item: getLocale(StringIds.ContextCreateSocialMediaCommentLong)
        }
      case Mojom.ActionType.PARAPHRASE:
        return {
          category: getLocale(StringIds.ContextRewrite),
          item: getLocale(StringIds.ContextParaphrase)
        }
      case Mojom.ActionType.IMPROVE:
        return {
          category: getLocale(StringIds.ContextRewrite),
          item: getLocale(StringIds.ContextImprove)
        }
      case Mojom.ActionType.ACADEMICIZE:
        return {
          category: getLocale(StringIds.ContextChangeTone),
          item: getLocale(StringIds.ContextAcademicize)
        }
      case Mojom.ActionType.PROFESSIONALIZE:
        return {
          category: getLocale(StringIds.ContextChangeTone),
          item: getLocale(StringIds.ContextProfessionalize)
        }
      case Mojom.ActionType.PERSUASIVE_TONE:
        return {
          category: getLocale(StringIds.ContextChangeTone),
          item: getLocale(StringIds.ContextPersuasiveTone)
        }
      case Mojom.ActionType.CASUALIZE:
        return {
          category: getLocale(StringIds.ContextChangeTone),
          item: getLocale(StringIds.ContextCasualize)
        }
      case Mojom.ActionType.FUNNY_TONE:
        return {
          category: getLocale(StringIds.ContextChangeTone),
          item: getLocale(StringIds.ContextFunnyTone)
        }
      case Mojom.ActionType.SHORTEN:
        return {
          category: getLocale(StringIds.ContextChangeLength),
          item: getLocale(StringIds.ContextShorten)
        }
      case Mojom.ActionType.EXPAND:
        return {
          category: getLocale(StringIds.ContextChangeLength),
          item: getLocale(StringIds.ContextExpand)
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
