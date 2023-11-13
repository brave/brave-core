/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'

import styles from './style.module.scss'
import DataContext from '../../state/context'
interface SiteTitleProps {
  size: "default" | "small"
}

function SiteTitle (props: SiteTitleProps) {
  const context = React.useContext(DataContext)

   return (
    <div className={classnames({
      [styles.box]: true,
      [styles.boxSm]: props.size === "small"
    })}>
      <div className={classnames({
        [styles.favIconContainer]: true,
        [styles.favIconContainerSm]: props.size === "small"
      })}>
        { context.favIconUrl && <img src={context.favIconUrl} /> }
      </div>
      <div className={classnames({
        [styles.titleContainer]: true,
        [styles.titleContainerSm]: props.size === "small"
      })}>
        <p className={styles.title} title={context.siteInfo?.title}>{context.siteInfo?.title}</p>
      </div>
    </div>
  )
}

export default SiteTitle
