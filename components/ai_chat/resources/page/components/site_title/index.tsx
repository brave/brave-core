/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'
interface SiteTitleProps {
  size: 'default' | 'small'
}

function SiteTitle(props: SiteTitleProps) {
  const context = useConversation()

  return (
    <div
      className={classnames({
        [styles.box]: true,
        [styles.boxSm]: props.size === 'small'
      })}
    >
      <div
        className={classnames({
          [styles.favIconContainer]: true,
          [styles.favIconContainerSm]: props.size === 'small'
        })}
      >
        { context.faviconUrl && <img src={context.faviconUrl} /> }
      </div>
      <div
        className={classnames({
          [styles.titleContainer]: true,
          [styles.titleContainerSm]: props.size === 'small'
        })}
      >
        <p
          className={styles.title}
          title={context.associatedContentInfo?.title}
        >
          {context.associatedContentInfo?.title}
        </p>
      </div>
    </div>
  )
}

export default SiteTitle
