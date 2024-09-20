/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'
import { showAlert } from '@brave/leo/react/alertCenter'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import FeedbackForm from '../feedback_form'
import styles from './style.module.scss'

interface ContextMenuAssistantProps {
  turnId: number
  isOpen: boolean
  onClick: () => void
  onClose: () => void
  onEditAnswerClicked: () => void
}

enum RatingStatus {
  Liked,
  Disliked,
  None
}

function ContextMenuAssistant_(
  props: ContextMenuAssistantProps,
  ref: React.RefObject<Map<number, Element>>
) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const {shouldDisableUserInput} = conversationContext
  const [feedbackId, setFeedbackId] = React.useState<string | null>()
  const [isFormVisible, setIsFormVisible] = React.useState(false)
  const [currentRatingStatus, setCurrentRatingStatus] =
    React.useState<RatingStatus>(RatingStatus.None)

  const formContainerElement = ref.current?.get(props.turnId)

  const hasSentRating = currentRatingStatus !== RatingStatus.None

  const handleLikeAnswer = () => {
    if (hasSentRating) return

    conversationContext.conversationHandler
      ?.rateMessage(true, props.turnId)
      .then((resp) => {
        if (!resp.ratingId) {
          showAlert({
            mode: 'simple',
            type: 'error',
            content: getLocale('ratingError'),
            actions: []
          })

          return
        }

        setCurrentRatingStatus(RatingStatus.Liked)
        showAlert({
          mode: 'simple',
          type: 'info',
          content: getLocale('answerLiked'),
          actions: []
        })
      })
  }

  const handleDislikeAnswer = () => {
    if (hasSentRating) return

    conversationContext.conversationHandler
      ?.rateMessage(false, props.turnId)
      .then((resp) => {
        if (!resp.ratingId) {
          showAlert({
            mode: 'simple',
            type: 'error',
            content: getLocale('ratingError'),
            actions: []
          })

          return
        }

        setFeedbackId(resp.ratingId)
        setCurrentRatingStatus(RatingStatus.Disliked)
        showAlert({
          mode: 'simple',
          type: 'info',
          content: getLocale('answerDisliked'),
          actions: [
            {
              text: getLocale('addFeedbackButtonLabel'),
              kind: 'plain-faint',
              action: () => setIsFormVisible(true)
            }
          ]
        }, 5000)
      })
  }

  const handleFormCancelClick = () => {
    setIsFormVisible(false)
  }

  const handleOnSubmit = (selectedCategory: string, feedbackText: string, shouldSendUrl: boolean) => {
    if (feedbackId) {
      conversationContext.conversationHandler
        ?.sendFeedback(selectedCategory, feedbackText, feedbackId, shouldSendUrl)
        .then((resp) => {
          if (!resp.isSuccess) {
            showAlert({
              mode: 'simple',
              type: 'error',
              content: getLocale('feedbackError'),
              actions: []
            })

            return
          }

          showAlert({
            mode: 'simple',
            type: 'success',
            content: getLocale('feedbackSent'),
            actions: []
          })
        })
      setIsFormVisible(false)
    }
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
            [styles.moreButtonHide]: aiChatContext.isMobile
          })}
        >
          <Icon name='more-vertical' />
        </Button>
        {!shouldDisableUserInput && (
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
        >
          <Icon name='thumb-up' />
          <span>{getLocale('likeAnswerButtonLabel')}</span>
        </leo-menu-item>
        <leo-menu-item
          class={classnames({
            [styles.disliked]: currentRatingStatus === RatingStatus.Disliked
          })}
          onClick={handleDislikeAnswer}
        >
          <Icon name='thumb-down' />
          <span>{getLocale('dislikeAnswerButtonLabel')}</span>
        </leo-menu-item>
      </ButtonMenu>
      {formContainerElement &&
        isFormVisible &&
        ReactDOM.createPortal(
          <FeedbackForm
            onSubmit={handleOnSubmit}
            onCancel={handleFormCancelClick}
            isDisabled={!Boolean(feedbackId).valueOf()}
          />,
          formContainerElement
        )}
    </>
  )
}

const ContextMenuAssistant = React.forwardRef(ContextMenuAssistant_)

export default ContextMenuAssistant
