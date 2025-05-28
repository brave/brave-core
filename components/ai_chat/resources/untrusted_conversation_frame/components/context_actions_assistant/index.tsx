/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import CopyButton from '../copy_button'
import { RegenerateAnswerMenu } from '../regenerate_answer_menu'
import styles from './style.module.scss'

const statuses = ['liked', 'disliked', 'none'] as const
type RatingStatus = (typeof statuses)[number]

interface ContextActionsAssistantProps {
  turnUuid?: string
  turnModelKey?: string
  onEditAnswerClicked: () => void
  onCopyTextClicked: () => void
}

export default function ContextActionsAssistant(
  props: ContextActionsAssistantProps,
) {
  const conversationContext = useUntrustedConversationContext()
  const [currentRatingStatus, setCurrentRatingStatus] =
    React.useState<RatingStatus>('none')

  const [isRegenerateAnswerMenuOpen, setIsRegenerateAnswerMenuOpen] =
    React.useState<boolean>(false)

  function handleLikeOrDislikeAnswer(status: RatingStatus) {
    if (!props.turnUuid) return
    setCurrentRatingStatus(status)
    conversationContext.parentUiFrame?.rateMessage(
      props.turnUuid,
      status === 'liked',
    )
  }

  function handleRegenerateAnswer(selectedModelKey: string) {
    if (!props.turnUuid) {
      return
    }
    conversationContext.conversationHandler?.regenerateAnswer(
      props.turnUuid,
      selectedModelKey,
    )
  }

  const leoModels = conversationContext.allModels.filter(
    (model) => model.options.leoModelOptions,
  )

  return (
    <div className={styles.actionsWrapper}>
      <CopyButton onClick={props.onCopyTextClicked} />
      <Button
        onClick={props.onEditAnswerClicked}
        fab
        size='small'
        kind='plain-faint'
        title={getLocale('editButtonLabel')}
        className={styles.button}
      >
        <Icon name='edit-pencil' />
      </Button>
      <Button
        onClick={() => handleLikeOrDislikeAnswer('liked')}
        fab
        size='small'
        kind='plain-faint'
        title={getLocale('likeAnswerButtonLabel')}
        className={styles.button}
      >
        <Icon
          name='thumb-up'
          className={classnames({
            [styles.liked]: currentRatingStatus === 'liked',
          })}
        />
      </Button>
      <Button
        onClick={() => handleLikeOrDislikeAnswer('disliked')}
        fab
        size='small'
        kind='plain-faint'
        title={getLocale('dislikeAnswerButtonLabel')}
        className={styles.button}
      >
        <Icon
          name='thumb-down'
          className={classnames({
            [styles.disliked]: currentRatingStatus === 'disliked',
          })}
        />
      </Button>
      {props.turnModelKey && (
        <RegenerateAnswerMenu
          isOpen={isRegenerateAnswerMenuOpen}
          onOpen={() => setIsRegenerateAnswerMenuOpen(true)}
          onClose={() => setIsRegenerateAnswerMenuOpen(false)}
          onRegenerate={handleRegenerateAnswer}
          leoModels={leoModels}
          turnModelKey={props.turnModelKey}
        />
      )}
    </div>
  )
}
