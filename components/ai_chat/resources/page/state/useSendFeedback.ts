// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { showAlert } from '@brave/leo/react/alertCenter'
import * as Mojom from '../../common/mojom'
import { getLocale } from '$web-common/locale'

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
  conversationHandler: Mojom.ConversationHandlerRemote,
  conversationEntriesFrameObserver: Mojom.ParentUIFrameCallbackRouter,
): SendFeedbackState {
  const feedbackId = React.useRef<string | null>(null)
  const [isFeedbackFormVisible, setIsFeedbackFormVisible] =
    React.useState(false)
  const [ratingTurnUuid, setRatingTurnUuid] = React.useState<{
    isLiked: boolean
    turnUuid: string
  }>()

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
      const response = await conversationHandler?.rateMessage(isLiked, turnUuid)
      if (!response.ratingId) {
        showAlert({
          type: 'error',
          content: getLocale('ratingError'),
          actions: [],
        })
        setRatingTurnUuid(undefined)
        return
      }
      if (isLiked) {
        showAlert({
          type: 'info',
          content: getLocale('answerLiked'),
          actions: [],
        })
      } else {
        // Allow the alert to stay for longer so that the user has time
        // to click the button to add feedback.
        feedbackId.current = response.ratingId
        showAlert(
          {
            type: 'info',
            content: getLocale('answerDisliked'),
            actions: [
              {
                text: getLocale('addFeedbackButtonLabel'),
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
    [conversationHandler, ratingTurnUuid],
  )

  // Listen to ratings requests from the child frame
  React.useEffect(() => {
    const listenerId =
      conversationEntriesFrameObserver.rateMessage.addListener(
        handleRateMessage,
      )

    return () => {
      conversationEntriesFrameObserver.removeListener(listenerId)
    }
  }, [conversationEntriesFrameObserver, handleRateMessage])

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
      const response = await conversationHandler?.sendFeedback(
        selectedCategory,
        feedbackText,
        feedbackId.current,
        shouldSendUrl,
      )
      if (!response.isSuccess) {
        showAlert({
          type: 'error',
          content: getLocale('feedbackError'),
          actions: [],
        })
        return
      }

      showAlert({
        type: 'success',
        content: getLocale('feedbackSent'),
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
