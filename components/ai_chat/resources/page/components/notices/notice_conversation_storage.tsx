// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import VisibilityTimer from '$web-common/visibilityTimer'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './notices.module.scss'

export default function NoticeConversationStorage() {
  const aiChatContext = useAIChat()

  const noticeElementRef = React.useRef<HTMLDivElement>(null)

  // Dismiss the notice for future loads after 4 seconds of being visible
  React.useEffect(() => {
    if (!noticeElementRef.current) {
      return
    }

    const visibilityTimer = new VisibilityTimer(
      aiChatContext.markStorageNoticeViewed,
      4000,
      noticeElementRef.current
    )

    visibilityTimer.startTracking()

    return () => {
      visibilityTimer.stopTracking()
    }
  }, [noticeElementRef.current])

  return (
    <div
      className={styles.notice}
      ref={noticeElementRef}
    >
      <div className={styles.illustration}>
        <Illustration />
      </div>
      <div className={styles.content}>
        <h4>{getLocale('noticeConversationHistoryTitle')}</h4>
        <p>{getLocale('noticeConversationHistoryBody')}</p>
        <p>
          <a
            href='#'
            target='_blank'
            onClick={() => aiChatContext.uiHandler?.openModelSupportUrl()}
          >
            {getLocale('learnMore')}
          </a>

        </p>
      </div>
      <Button
        className={styles.closeButton}
        kind='plain-faint'
        fab
        title={getLocale('closeNotice')}
        onClick={aiChatContext.dismissStorageNotice}
      >
        <Icon name='close' />
      </Button>
    </div>
  )
}

function Illustration() {
  return (
    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 216 136">
    <rect width="210" height="130" x="6" y="6" fill="#000" rx="8"></rect>
    <rect
      width="211"
      height="131"
      x="0.5"
      y="0.5"
      fill="#fff"
      stroke="#000"
      rx="8.5"
    ></rect>
    <path
      fill="#EDEDF2"
      stroke="#000"
      d="M71.5 1V.5H9A8.5 8.5 0 0 0 .5 9v114a8.5 8.5 0 0 0 8.5 8.5h62.5z"
    ></path>
    <rect width="54" height="8" x="9" y="27" fill="#5E5E62" rx="4"></rect>
    <rect width="54" height="8" x="11" y="47" fill="#000" rx="4"></rect>
    <rect width="54" height="8" x="9" y="45" fill="#4A46E0" rx="4"></rect>
    <rect width="54" height="8" x="9" y="63" fill="#5E5E62" rx="4"></rect>
    <rect width="54" height="8" x="9" y="81" fill="#5E5E62" rx="4"></rect>
    <rect width="54" height="8" x="9" y="99" fill="#5E5E62" rx="4"></rect>
    <path
      fill="#5E5E62"
      fillRule="evenodd"
      d="M11.385 15.615A.385.385 0 0 0 11 16a5 5 0 1 0 1.848-3.882 1.304 1.304 0 1 0 .58.522A4.23 4.23 0 1 1 11.77 16a.385.385 0 0 0-.385-.385m7.307.385h-2.026a.8.8 0 0 0-.281-.282v-1.641a.385.385 0 0 0-.77 0v1.641a.769.769 0 1 0 1.051 1.051h2.026a.385.385 0 0 0 0-.769"
      clipRule="evenodd"
    ></path>
    <rect width="60" height="8" x="109" y="23" fill="#C7C7CC" rx="4"></rect>
    <rect width="8" height="8" x="109" y="11" fill="#C7C7CC" rx="4"></rect>
    <rect width="32" height="8" x="109" y="111" fill="#C7C7CC" rx="4"></rect>
    <rect width="8" height="8" x="109" y="99" fill="#C7C7CC" rx="4"></rect>
    <rect width="92" height="8" x="109" y="55" fill="#C7C7CC" rx="4"></rect>
    <rect width="78" height="8" x="109" y="67" fill="#C7C7CC" rx="4"></rect>
    <rect width="68" height="8" x="109" y="79" fill="#C7C7CC" rx="4"></rect>
    <rect width="8" height="8" x="109" y="43" fill="url(#a)" rx="4"></rect>
    <rect
      width="49"
      height="49"
      x="46.5"
      y="41.5"
      fill="#4A46E0"
      stroke="#000"
      rx="24.5"
    ></rect>
    <path
      fill="#fff"
      fillRule="evenodd"
      d="M57.923 64.91a1.09 1.09 0 0 0-1.09 1.09c0 7.824 6.343 14.167 14.167 14.167S85.167 73.824 85.167 66 78.824 51.833 71 51.833c-3.319 0-6.449 1.15-8.931 3.169a3.695 3.695 0 1 0 1.642 1.48A11.95 11.95 0 0 1 71 54.011c6.62 0 11.987 5.368 11.987 11.988S77.62 77.986 71 77.986 59.013 72.62 59.013 66a1.09 1.09 0 0 0-1.09-1.09M78.628 66h-5.74a2.2 2.2 0 0 0-.798-.798v-4.65a1.09 1.09 0 1 0-2.18 0v4.65A2.179 2.179 0 0 0 71 69.269c.807 0 1.511-.438 1.888-1.09h5.74a1.09 1.09 0 1 0 0-2.179"
      clipRule="evenodd"
    ></path>
    <defs>
      <linearGradient
        id="a"
        x1="116.584"
        x2="108.993"
        y1="50.447"
        y2="43.008"
        gradientUnits="userSpaceOnUse"
      >
        <stop offset="0.026" stopColor="#FA7250"></stop>
        <stop offset="0.401" stopColor="#FF1893"></stop>
        <stop offset="0.995" stopColor="#A78AFF"></stop>
      </linearGradient>
    </defs>
  </svg>
  )
}
