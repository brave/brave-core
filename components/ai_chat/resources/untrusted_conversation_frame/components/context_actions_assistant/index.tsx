/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import classnames from '$web-common/classnames'
import { AUTOMATIC_MODEL_KEY } from '../../../common/constants'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import CopyButton from '../copy_button'
import { RegenerateAnswerMenu } from '../regenerate_answer_menu'
import styles from './style.module.scss'

const statuses = ['liked', 'disliked', 'none'] as const
type RatingStatus = (typeof statuses)[number]

interface ContextActionsAssistantProps {
  turnUuid?: string
  turnModelKey?: string
  turnNEARVerified?: boolean
  onEditAnswerClicked?: () => void
  onCopyTextClicked?: () => void
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

  const handleRegenerateAnswer = React.useCallback(
    (selectedModelKey: string) => {
      if (!props.turnUuid) {
        return
      }
      const modelToBeUsed = conversationContext.allModels.find(
        (model) => model.key === selectedModelKey,
      )
      // If the user is a non-premium user and the model is premium, we need to
      // show the premium suggestion.
      if (
        !conversationContext.isPremiumUser
        && modelToBeUsed?.options.leoModelOptions?.access
          === Mojom.ModelAccess.PREMIUM
      ) {
        conversationContext.setShowPremiumSuggestionForRegenerate(
          true,
        )
        return
      }

      // Reset the premium suggestion if the user regenerates with a
      // non-premium model.
      conversationContext.setShowPremiumSuggestionForRegenerate(
        false,
      )
      conversationContext.conversationHandler?.regenerateAnswer(
        props.turnUuid,
        selectedModelKey,
      )
    },
    [conversationContext, props.turnUuid],
  )

  const isSummaryResponseTurn = React.useMemo(() => {
    if (!props.turnUuid) return false
    const history = conversationContext.conversationHistory
    const turnIndex = history.findIndex((t) => t.uuid === props.turnUuid)
    if (turnIndex <= 0) return false
    const prev = history[turnIndex - 1]
    const curr = history[turnIndex]
    return !!(
      curr?.characterType === Mojom.CharacterType.ASSISTANT
      && prev?.characterType === Mojom.CharacterType.HUMAN
      && (prev.actionType === Mojom.ActionType.SUMMARIZE_PAGE
        || prev.actionType === Mojom.ActionType.SUMMARIZE_VIDEO)
    )
  }, [props.turnUuid, conversationContext.conversationHistory])

  const leoModels = React.useMemo(
    () =>
      conversationContext.allModels.filter((model) => {
        if (!model.options.leoModelOptions) {
          return false
        }
        if (model.key === AUTOMATIC_MODEL_KEY) {
          return false
        }
        const isSummaryCategory =
          model.options.leoModelOptions.category === Mojom.ModelCategory.SUMMARY
        if (isSummaryCategory && !isSummaryResponseTurn) {
          return false
        }
        return true
      }),
    [conversationContext.allModels, isSummaryResponseTurn],
  )

  const handleOpenCloseRegenerateAnswerMenu = (isOpen: boolean) => {
    setIsRegenerateAnswerMenuOpen(isOpen)
  }

  return (
    <div className={styles.actionsWrapper}>
      {props.onCopyTextClicked && (
        <CopyButton onClick={props.onCopyTextClicked} />
      )}
      {props.onEditAnswerClicked && (
        <Button
          onClick={props.onEditAnswerClicked}
          fab
          size='small'
          kind='plain-faint'
          title={getLocale(S.CHAT_UI_EDIT_BUTTON_LABEL)}
          className={styles.button}
        >
          <Icon name='edit-pencil' />
        </Button>
      )}
      <Button
        onClick={() => handleLikeOrDislikeAnswer('liked')}
        fab
        size='small'
        kind='plain-faint'
        title={getLocale(S.CHAT_UI_LIKE_ANSWER_BUTTON_LABEL)}
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
        title={getLocale(S.CHAT_UI_DISMISS_BUTTON_LABEL)}
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
          onOpen={() => handleOpenCloseRegenerateAnswerMenu(true)}
          onClose={() => handleOpenCloseRegenerateAnswerMenu(false)}
          onRegenerate={handleRegenerateAnswer}
          leoModels={leoModels}
          turnModelKey={props.turnModelKey}
          turnNEARVerified={props.turnNEARVerified}
          isPremiumUser={conversationContext.isPremiumUser}
        />
      )}
    </div>
  )
}
