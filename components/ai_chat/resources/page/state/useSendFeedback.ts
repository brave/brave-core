// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { showAlert } from '@brave/leo/react/alertCenter'
import { getLocale } from '$web-common/locale'
import { ConversationAPI } from '../api/conversation_api'
import { useAIChat } from './ai_chat_context'

/**
 * State needed for UI to embed a feedback form
 */
export interface SendFeedbackState {
  isFeedbackFormVisible: boolean
  ratingTurnUuid?: { isLiked: boolean; turnUuid: string }
  handleFeedbackFormCancel: () => void
  handleFeedbackFormSubmit: (
    selectedCategory: string,
    feedbackText: string,
    shouldSendUrl: boolean,
  ) => Promise<void>
  handleCloseRateMessagePrivacyModal: () => void
  handleRateMessage: (
    turnUuid: string,
    isLiked: boolean,
    ignoreFutureWarnings?: boolean,
  ) => Promise<void>
}

export const defaultSendFeedbackState: SendFeedbackState = {
  isFeedbackFormVisible: false,
  ratingTurnUuid: undefined,
  handleFeedbackFormCancel: () => {},
  handleFeedbackFormSubmit: async () => {},
  handleCloseRateMessagePrivacyModal: () => {},
  handleRateMessage: async () => {},
}

/**
 * This hook handles calls from the child frame to rate a message and provides
 * handling of the form to collect more user feedback, actioned via the alert
 * center. This is self-contained apart from the form itself which should
 * be implemented in the parent frame UI.
 */
export default function useSendFeedback(
  api: ConversationAPI,
): SendFeedbackState {
  const aiChat = useAIChat()

  const feedbackId = React.useRef<string | null>(null)
  const [isFeedbackFormVisible, setIsFeedbackFormVisible] =
    React.useState(false)
  const [ratingTurnUuid, setRatingTurnUuid] = React.useState<{
    isLiked: boolean
    turnUuid: string
  }>()

  // Only allows a rating to be sent once the user has accepted
  // the privacy dialog. When called a second time, it will
  // send the rating. To cancel, set handleCloseRateMessagePrivacyModal.
  // Also further allows a feedback form after a negative rating.
  const handleRateMessage = React.useCallback(
    async (turnUuid: string, isLiked: boolean) => {
      // Reset feedback form
      feedbackId.current = null
      setIsFeedbackFormVisible(false)

      // If ratingTurnUuid is undefined, set the ratingTurnUuid
      // to display the rate message privacy modal.
      if (!ratingTurnUuid) {
        setRatingTurnUuid({ isLiked, turnUuid })
        return
      }

      // Rate the message.
      const response = await api.actions.rateMessage(isLiked, turnUuid)
      if (!response.ratingId) {
        showAlert({
          type: 'error',
          content: getLocale(S.CHAT_UI_RATING_ERROR),
          actions: [],
        })
        setRatingTurnUuid(undefined)
        return
      }
      if (isLiked) {
        showAlert({
          type: 'info',
          content: getLocale(S.CHAT_UI_ANSWER_LIKED),
          actions: [],
        })
      } else {
        // Allow the alert to stay for longer so that the user has time
        // to click the button to add feedback.
        feedbackId.current = response.ratingId
        showAlert(
          {
            type: 'info',
            content: getLocale(S.CHAT_UI_ANSWER_DISLIKED),
            actions: [
              {
                text: getLocale(S.CHAT_UI_ADD_FEEDBACK_BUTTON_LABEL),
                kind: 'plain-faint',
                action: () => setIsFeedbackFormVisible(true),
              },
            ],
          },
          5000,
        )
      }
      setRatingTurnUuid(undefined)
    },
    [api, ratingTurnUuid],
  )

  // Listen to ratings requests from the child frame
  aiChat.api.useRateMessage(handleRateMessage, [])

  function handleFeedbackFormCancel() {
    setIsFeedbackFormVisible(false)
  }

  function handleCloseRateMessagePrivacyModal() {
    setRatingTurnUuid(undefined)
  }

  async function handleFeedbackFormSubmit(
    selectedCategory: string,
    feedbackText: string,
    shouldSendUrl: boolean,
  ) {
    if (feedbackId.current) {
      const response = await api.actions.sendFeedback(
        selectedCategory,
        feedbackText,
        feedbackId.current,
        shouldSendUrl,
      )
      if (!response.isSuccess) {
        showAlert({
          type: 'error',
          content: getLocale(S.CHAT_UI_FEEDBACK_SUBMIT_ERROR),
          actions: [],
        })
        return
      }

      showAlert({
        type: 'success',
        content: getLocale(S.CHAT_UI_FEEDBACK_SENT),
        actions: [],
      })
      setIsFeedbackFormVisible(false)
    }
  }

  return {
    isFeedbackFormVisible,
    ratingTurnUuid,
    handleFeedbackFormCancel,
    handleFeedbackFormSubmit,
    handleCloseRateMessagePrivacyModal,
    handleRateMessage,
  }
}
