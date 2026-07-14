/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Dialog from '@brave/leo/react/dialog'
import { getLocale, formatLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

const CATEGORY_OPTIONS = new Map([
  ['not-helpful', getLocale(S.CHAT_UI_OPTION_NOT_HELPFUL)],
  ['incorrect', getLocale(S.CHAT_UI_OPTION_INCORRECT)],
  ['unsafe-harmful', getLocale(S.CHAT_UI_OPTION_UNSAFE_HARMFUL)],
  ['other', getLocale(S.CHAT_UI_OPTION_OTHER)],
])

const getHostName = (url: string) => {
  try {
    return new URL(url).hostname
  } catch (e) {
    return ''
  }
}

function FeedbackForm() {
  const [category, setCategory] = React.useState('')
  const [feedbackText, setFeedbackText] = React.useState('')
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const [shouldSendUrl, setShouldSendUrl] = React.useState(true)

  const canSubmit = !!category

  const handleSubmit = () => {
    conversationContext.handleFeedbackFormSubmit(
      category,
      feedbackText,
      shouldSendUrl,
    )
  }

  const handleInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setFeedbackText(e.target.value)
  }

  const handleCheckboxChange = ({ checked }: { checked: boolean }) => {
    setShouldSendUrl(checked)
  }

  // Reset form state when the dialog closes so the next open is clean.
  React.useEffect(() => {
    if (!conversationContext.isFeedbackFormVisible) {
      setCategory('')
      setFeedbackText('')
      setShouldSendUrl(true)
    }
  }, [conversationContext.isFeedbackFormVisible])

  return (
    <Dialog
      isOpen={conversationContext.isFeedbackFormVisible}
      showClose
      onClose={conversationContext.handleFeedbackFormCancel}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.dialogTitle}
      >
        {getLocale(S.CHAT_UI_PROVIDE_FEEDBACK_TITLE)}
      </div>
      <div className={styles.body}>
        <div className={styles.categorySection}>
          <div className={styles.categoryLabel}>
            {getLocale(S.CHAT_UI_FEEDBACK_CATEGORY_LABEL)}
          </div>
          <div className={styles.reasons}>
            {[...CATEGORY_OPTIONS.keys()].map((key) => (
              <Button
                key={key}
                kind={category === key ? 'filled' : 'outline'}
                onClick={() => setCategory(key)}
              >
                {CATEGORY_OPTIONS.get(key)}
              </Button>
            ))}
          </div>
        </div>
        <label className={styles.detailsLabel}>
          {getLocale(S.CHAT_UI_FEEDBACK_DESCRIPTION_LABEL)}
          <textarea
            value={feedbackText}
            onChange={handleInputChange}
            placeholder={getLocale(S.CHAT_UI_FEEDBACK_DESCRIPTION_LABEL)}
          />
        </label>
        {conversationContext.associatedContentInfo.length > 0 && (
          <Checkbox
            checked={shouldSendUrl}
            onChange={handleCheckboxChange}
          >
            <span>
              {formatLocale(S.CHAT_UI_SEND_SITE_HOSTNAME_LABEL, {
                $1: conversationContext.associatedContentInfo
                  .map((c) => getHostName(c.url.url))
                  .join(', '),
              })}
            </span>
          </Checkbox>
        )}
        {!aiChatContext.isPremiumUser && (
          <div className={styles.premiumNote}>
            {formatLocale(S.CHAT_UI_FEEDBACK_PREMIUM_NOTE, {
              $1: (linkText) => (
                <a
                  href='#'
                  className={styles.learnMoreLink}
                  onClick={(e) => {
                    e.preventDefault()
                    aiChatContext.goPremium()
                  }}
                >
                  {linkText}
                </a>
              ),
            })}
          </div>
        )}
        <div className={styles.footer}>
          <Button
            className={styles.footerButton}
            kind='plain-faint'
            size='medium'
            onClick={conversationContext.handleFeedbackFormCancel}
          >
            {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
          </Button>
          <Button
            className={styles.footerButton}
            kind='filled'
            size='medium'
            isDisabled={!canSubmit}
            onClick={handleSubmit}
          >
            {getLocale(S.CHAT_UI_SUBMIT_BUTTON_LABEL)}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}

export default FeedbackForm
