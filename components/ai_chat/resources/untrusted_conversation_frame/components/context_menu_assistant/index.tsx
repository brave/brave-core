/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
// import * as ReactDOM from 'react-dom'
// import { showAlert } from '@brave/leo/react/alertCenter'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'

enum RatingStatus {
  Liked,
  Disliked,
  None
}

interface ContextMenuAssistantProps {
  turnUuid?: string
  isOpen: boolean
  onClick: () => void
  onClose: () => void
  onEditAnswerClicked: () => void
}

export default function ContextMenuAssistant(props: ContextMenuAssistantProps) {
  const conversationContext = useUntrustedConversationContext()
  const [currentRatingStatus, setCurrentRatingStatus] =
    React.useState<RatingStatus>(RatingStatus.None)

  const hasSentRating = currentRatingStatus !== RatingStatus.None

  function handleLikeAnswer() {
    if (!props.turnUuid) return
    if (hasSentRating) return
    setCurrentRatingStatus(RatingStatus.Liked)
    conversationContext.parentUiFrame?.rateMessage(props.turnUuid, true)
  }

  function handleDislikeAnswer() {
    if (!props.turnUuid) return
    if (hasSentRating) return
    setCurrentRatingStatus(RatingStatus.Disliked)
    conversationContext.parentUiFrame?.rateMessage(props.turnUuid, false)
  }

  return (
    <>
      <ButtonMenu
        className={styles.buttonMenu}
        isOpen={props.isOpen}
        onClose={props.onClose}
      >
        <Button
          fab
          slot='anchor-content'
          size="tiny"
          kind="plain-faint"
          onClick={props.onClick}
          className={classnames({
            [styles.moreButton]: true,
            [styles.moreButtonActive]: props.isOpen,
            [styles.moreButtonHide]: conversationContext.isMobile
          })}
        >
          <Icon name='more-vertical' />
        </Button>
        {conversationContext.canSubmitUserEntries && (
          <leo-menu-item
            onClick={props.onEditAnswerClicked}
          >
            <Icon name='edit-pencil' />
            <span>{getLocale('editAnswerLabel')}</span>
          </leo-menu-item>
        )}
        <leo-menu-item
          class={classnames({
            [styles.liked]: currentRatingStatus === RatingStatus.Liked
          })}
          onClick={handleLikeAnswer}
          title={getLocale('likeDislikeAnswerButtonTitle')}
        >
          <Icon name='thumb-up' />
          <span>{getLocale('likeAnswerButtonLabel')}</span>
        </leo-menu-item>
        <leo-menu-item
          class={classnames({
            [styles.disliked]: currentRatingStatus === RatingStatus.Disliked
          })}
          onClick={handleDislikeAnswer}
          title={getLocale('likeDislikeAnswerButtonTitle')}
        >
          <Icon name='thumb-down' />
          <span>{getLocale('dislikeAnswerButtonLabel')}</span>
        </leo-menu-item>
      </ButtonMenu>
    </>
  )
}
