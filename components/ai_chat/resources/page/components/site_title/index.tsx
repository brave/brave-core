/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import styles from './style.module.scss'
import { SiteInfo } from '../../api/page_handler'

interface SiteTitleProps {
  siteInfo?: SiteInfo
  favIconUrl: string | null
}

function SiteTitle (props: SiteTitleProps) {
   return (
    <div className={styles.box}>
      <div className={styles.favIconBox}>
        <div className={styles.imgBox}>
          { props.favIconUrl && <img src={props.favIconUrl} /> }
        </div>
      </div>
      <div className={styles.titleBox}>
        <p className={styles.title}>{props.siteInfo?.title}</p>
      </div>
    </div>
  )
}

export default SiteTitle
