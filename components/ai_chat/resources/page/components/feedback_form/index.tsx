/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DropDown from '@brave/leo/react/dropdown'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import { getLocale , formatLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

const CATEGORY_OPTIONS = new Map([
  ['not-helpful', getLocale(S.CHAT_UI_OPTION_NOT_HELPFUL)],
  ['incorrect', getLocale(S.CHAT_UI_OPTION_INCORRECT)],
  ['unsafe-harmful', getLocale(S.CHAT_UI_OPTION_UNSAFE_HARMFUL)],
  ['other', getLocale(S.CHAT_UI_OPTION_OTHER)]
])

const getHostName = (url: string) => {
  try {
    return new URL(url).hostname
  } catch (e) {
    return ''
  }
}

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
      <h4>{getLocale(S.CHAT_UI_PROVIDE_FEEDBACK_TITLE)}</h4>
      <form>
        <fieldset>
          <DropDown
            className={styles.dropdown}
            placeholder={getLocale(S.CHAT_UI_SELECT_FEEDBACK_TOPIC)}
            onChange={handleSelectOnChange}
            required={true}
            value={CATEGORY_OPTIONS.get(category)}
          >
            <div slot='label'>{getLocale(S.CHAT_UI_FEEDBACK_CATEGORY_LABEL)}</div>
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
            {getLocale(S.CHAT_UI_FEEDBACK_DESCRIPTION_LABEL)}
            <textarea
              onChange={handleInputChange}
              placeholder={getLocale(S.CHAT_UI_FEEDBACK_DESCRIPTION_LABEL)}
            />
          </label>
        </fieldset>
        {conversationContext.associatedContentInfo.length > 0 && (
          <fieldset>
            <Checkbox checked={shouldSendUrl} onChange={handleCheckboxChange}>
              <label>{
                formatLocale(S.CHAT_UI_SEND_SITE_HOSTNAME_LABEL, {
                  $1: conversationContext.associatedContentInfo.map(c => getHostName(c.url.url)).join(', ')
                })
              }</label>
            </Checkbox>
          </fieldset>
        )}
        {!aiChatContext.isPremiumUser && (
          <div className={styles.premiumNote}>
            {formatLocale(S.CHAT_UI_FEEDBACK_PREMIUM_NOTE, {
              $1: (linkText) => (
                  <Button kind='plain' size='medium' onClick={aiChatContext.goPremium}>
                    {linkText}
                  </Button>
                )
            })}
          </div>
        )}
        <fieldset className={styles.actions}>
          <Button onClick={conversationContext.handleFeedbackFormCancel} kind='plain-faint'>
            {getLocale(S.CHAT_UI_CANCEL_BUTTON_LABEL)}
          </Button>
          <Button isDisabled={!canSubmit} onClick={handleSubmit}>
            {getLocale(S.CHAT_UI_SUBMIT_BUTTON_LABEL)}
          </Button>
        </fieldset>
      </form>
    </div>
  )
}

export default FeedbackForm
