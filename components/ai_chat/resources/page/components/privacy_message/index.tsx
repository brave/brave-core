/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import { getLocale, formatLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './style.module.scss'

const WIKI_URL =
  'https://support.brave.app/hc/en-us/articles/26727364100493-What-are-the-differences-between-Leo-s-AI-Models'
const PRIVACY_URL = 'https://brave.com/privacy/browser/#brave-leo'

function PrivacyMessage() {
  const context = useAIChat()
  const buttonRef = React.useRef<HTMLButtonElement>()

  const handleLinkClick = (e: React.MouseEvent, url: string) => {
    e.preventDefault()
    const mojomUrl = new Url()
    mojomUrl.url = url

    context.api.actions.uiHandler.openURL(mojomUrl)
  }

  const createLinkWithClickHandler = (content: string, url: string) => (
    <a
      onClick={(e) => handleLinkClick(e, url)}
      href={url}
      target='_blank'
    >
      {content}
    </a>
  )

  const aboutDescription = formatLocale(S.CHAT_UI_ABOUT_DESCRIPTION, {
    $1: (content) => createLinkWithClickHandler(content, WIKI_URL),
  })

  const aboutDescription3 = formatLocale(S.CHAT_UI_ABOUT_DESCRIPTION_3, {
    $1: (content) => createLinkWithClickHandler(content, PRIVACY_URL),
  })

  React.useEffect(() => {
    const button = buttonRef.current
    if (button === undefined) return
    setTimeout(() => button.focus())
  }, [])

  return (
    <Dialog
      isOpen={true}
      escapeCloses={false}
      backdropClickCloses={false}
      className={styles.dialog}
    >
      <div slot='subtitle'>{getLocale(S.CHAT_UI_PRIVACY_TITLE)}</div>
      <div className={styles.content}>
        <p>{aboutDescription}</p>
        <p>{getLocale(S.CHAT_UI_ABOUT_DESCRIPTION_2)}</p>
        <p>{aboutDescription3}</p>
      </div>
      <div slot='actions'>
        <Button
          ref={buttonRef}
          onClick={context.handleAgreeClick}
        >
          {getLocale(S.CHAT_UI_ACCEPT_BUTTON_LABEL)}
        </Button>
      </div>
    </Dialog>
  )
}

export default PrivacyMessage
