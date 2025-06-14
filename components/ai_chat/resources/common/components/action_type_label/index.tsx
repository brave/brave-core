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
        return { category: undefined, item: getLocale(S.AI_CHAT_CONTEXT_SUMMARIZE_TEXT)}
      case Mojom.ActionType.EXPLAIN:
        return { category: undefined, item: getLocale(S.AI_CHAT_CONTEXT_EXPLAIN)}
      case Mojom.ActionType.CREATE_TAGLINE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CREATE),
          item: getLocale(S.AI_CHAT_CONTEXT_CREATE_TAGLINE)
        }
      case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_SHORT:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST),
          item: getLocale(S.AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT)
        }
      case Mojom.ActionType.CREATE_SOCIAL_MEDIA_COMMENT_LONG:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST),
          item: getLocale(S.AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG)
        }
      case Mojom.ActionType.PARAPHRASE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_REWRITE),
          item: getLocale(S.AI_CHAT_CONTEXT_PARAPHRASE)
        }
      case Mojom.ActionType.IMPROVE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_REWRITE),
          item: getLocale(S.AI_CHAT_CONTEXT_IMPROVE)
        }
      case Mojom.ActionType.ACADEMICIZE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_TONE),
          item: getLocale(S.AI_CHAT_CONTEXT_ACADEMICIZE)
        }
      case Mojom.ActionType.PROFESSIONALIZE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_TONE),
          item: getLocale(S.AI_CHAT_CONTEXT_PROFESSIONALIZE)
        }
      case Mojom.ActionType.PERSUASIVE_TONE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_TONE),
          item: getLocale(S.AI_CHAT_CONTEXT_PERSUASIVE_TONE)
        }
      case Mojom.ActionType.CASUALIZE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_TONE),
          item: getLocale(S.AI_CHAT_CONTEXT_CASUALIZE)
        }
      case Mojom.ActionType.FUNNY_TONE:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_TONE),
          item: getLocale(S.AI_CHAT_CONTEXT_FUNNY_TONE)
        }
      case Mojom.ActionType.SHORTEN:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_LENGTH),
          item: getLocale(S.AI_CHAT_CONTEXT_SHORTEN)
        }
      case Mojom.ActionType.EXPAND:
        return {
          category: getLocale(S.AI_CHAT_CONTEXT_CHANGE_LENGTH),
          item: getLocale(S.AI_CHAT_CONTEXT_EXPAND)
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
