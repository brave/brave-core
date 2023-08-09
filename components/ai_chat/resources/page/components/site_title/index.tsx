/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'
import { SiteInfo } from '../../api/page_handler'

interface SiteTitleProps {
  siteInfo?: SiteInfo
  favIconUrl?: string
  onDisconnectClick?: () => void
}

function SiteTitle (props: SiteTitleProps) {
   return (
    <div className={styles.box} title="Page content will be sent to Brave Leo along with your messages. Click the disconnect button on the right to start a new conversation about a different topic.">
      <div className={styles.favIconBox}>
        { props.favIconUrl && <img src={props.favIconUrl} /> }
      </div>
      <div className={styles.titleBox}>
        <p className={styles.title}>{props.siteInfo?.title}</p>
      </div>
      <div className={styles.actions}>
        <Button kind="plain" onClick={props.onDisconnectClick}>
          <Icon name="plug-connected-x" />
        </Button>
      </div>
    </div>
  )
}

export default SiteTitle
