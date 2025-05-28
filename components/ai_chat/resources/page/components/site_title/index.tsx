/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'
import { AssociatedContent } from 'components/ai_chat/resources/common/mojom'

interface SiteTitleProps {
  size: 'default' | 'small'
  content: AssociatedContent
}

function SiteTitle(props: SiteTitleProps) {
  return (
    <div
      className={classnames({
        [styles.box]: true,
        [styles.boxSm]: props.size === 'small',
      })}
    >
      <div
        className={classnames({
          [styles.favIconContainer]: true,
          [styles.favIconContainerSm]: props.size === 'small',
        })}
      >
        {props.content.url?.url && (
          <img
            src={`chrome://favicon2?size=64&pageUrl=${encodeURIComponent(props.content.url.url)}`}
          />
        )}
      </div>
      <div
        className={classnames({
          [styles.titleContainer]: true,
          [styles.titleContainerSm]: props.size === 'small',
        })}
      >
        <p
          className={styles.title}
          title={props.content.title}
        >
          {props.content.title}
        </p>
      </div>
    </div>
  )
}

export default function SiteTitles(props: { size: 'default' | 'small' }) {
  const context = useConversation()
  return (
    <div
      className={classnames({
        [styles.container]: true,
        [styles.small]: props.size === 'small',
      })}
    >
      {context.associatedContentInfo.map((c) => (
        <SiteTitle
          key={c.uuid}
          size={props.size}
          content={c}
        />
      ))}
    </div>
  )
}
