/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DropDown from '@brave/leo/react/dropdown'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import { getLocale } from '$web-common/locale'
import formatMessage from '$web-common/formatMessage'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

const CATEGORY_OPTIONS = new Map([
  ['not-helpful', getLocale('optionNotHelpful')],
  ['incorrect', getLocale('optionIncorrect')],
  ['unsafe-harmful', getLocale('optionUnsafeHarmful')],
  ['other', getLocale('optionOther')]
])

function FeedbackForm() {
  const ref = React.useRef<HTMLDivElement>(null)
  const [category, setCategory] = React.useState('')
  const [feedbackText, setFeedbackText] = React.useState('')
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const [shouldSendUrl, setShouldSendUrl] = React.useState(true)

  const canSubmit = !!category

  const handleSubmit = () => {
    conversationContext.handleFeedbackFormSubmit(category, feedbackText, shouldSendUrl)
  }

  const handleSelectOnChange = ({ value }: { value: string }) => {
    setCategory(value)
  }

  const handleInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setFeedbackText(e.target.value)
  }

  const handleCheckboxChange = ({ checked }: { checked: boolean; }) => {
    setShouldSendUrl(checked)
  }

  React.useEffect(() => {
    ref.current?.scrollIntoView()
  }, [])

  // TODO(petemill): allow submission and show error on Dropdown if nothing
  // is selected when user attempts to Submit.

  return (
    <div ref={ref} className={styles.form}>
      <h4>{getLocale('provideFeedbackTitle')}</h4>
      <form>
        <fieldset>
          <DropDown
            className={styles.dropdown}
            placeholder={getLocale('selectFeedbackTopic')}
            onChange={handleSelectOnChange}
            required={true}
            value={CATEGORY_OPTIONS.get(category)}
          >
            <div slot='label'>{getLocale('feedbackCategoryLabel')}</div>
            {[...CATEGORY_OPTIONS.keys()].map((key) => {
              return (
                <leo-option key={key} value={key}>
                  {CATEGORY_OPTIONS.get(key)}
                </leo-option>
              )
            })}
          </DropDown>
        </fieldset>
        <fieldset>
          <label>
            {getLocale('feedbackDescriptionLabel')}
            <textarea
              onChange={handleInputChange}
              placeholder={getLocale('feedbackDescriptionLabel')}
            />
          </label>
        </fieldset>
        {conversationContext.associatedContentInfo?.hostname && (
          <fieldset>
            <Checkbox checked={shouldSendUrl} onChange={handleCheckboxChange}>
              <label>{
                formatMessage(getLocale('sendSiteHostnameLabel'), {
                  placeholders: {
                    $1: conversationContext.associatedContentInfo.hostname
                  }
                })
              }</label>
            </Checkbox>
          </fieldset>
        )}
        {!aiChatContext.isPremiumUser && (
          <div className={styles.premiumNote}>
            {formatMessage(getLocale('feedbackPremiumNote'), {
              tags: {
                $1: (linkText) => (
                  <Button kind='plain' size='medium' onClick={aiChatContext.goPremium}>
                    {linkText}
                  </Button>
                )
              }
            })}
          </div>
        )}
        <fieldset className={styles.actions}>
          <Button onClick={conversationContext.handleFeedbackFormCancel} kind='plain-faint'>
            {getLocale('cancelButtonLabel')}
          </Button>
          <Button isDisabled={!canSubmit} onClick={handleSubmit}>
            {getLocale('submitButtonLabel')}
          </Button>
        </fieldset>
      </form>
    </div>
  )
}

export default FeedbackForm
