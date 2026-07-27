// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import ConversationAreaButton from '../../../common/components/conversation_area_button'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'

interface Props {
  threadUuid: string
}

// A pill shown in the footer of an AI answer that has a child thread. Clicking
// it asks the parent (trusted) frame to open the thread panel.
export default function ThreadIndicator(props: Props) {
  const context = useUntrustedConversationContext()

  const thread = context.threads.find((t) => t.uuid === props.threadUuid)
  const replyCount = thread?.entryCount ?? 0

  const repliesLabel =
    context.api.useGetPluralString(
      S.CHAT_UI_THREAD_REPLIES_LABEL,
      replyCount,
    ).data ?? ''

  const label =
    replyCount === 0 ? getLocale(S.CHAT_UI_THREAD_EMPTY_LABEL) : repliesLabel

  return (
    <ConversationAreaButton
      className={styles.threadIndicator}
      icon={<span aria-hidden='true'>🧵</span>}
      onClick={() => context.parentUiFrame?.openThread(props.threadUuid)}
    >
      <span className={styles.label}>{label}</span>
      <Icon
        name='arrow-right'
        className={styles.arrow}
      />
    </ConversationAreaButton>
  )
}
