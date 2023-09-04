/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale, splitStringForTag } from '$web-common/locale'

import styles from './style.module.scss'
import privacyGraphicUrl from '../../assets/privacy_graphic.svg'
import getPageHandlerInstance from '../../api/page_handler'

function PrivacyMessage () {
  const aboutDescription2 = splitStringForTag(getLocale('aboutDescription_2'))

  const handleAnchorClick = () => {
    getPageHandlerInstance().pageHandler.openBraveLeoWiki()
  }

  return (
    <div className={styles.box}>
      <figure>
        <img src={privacyGraphicUrl}
          alt="Illustration: window and chat bubble, representing communication" />
      </figure>
      <section>
        <b>{getLocale('aboutTitle')}</b>
        <p>{getLocale('aboutDescription')}</p>
        <p>
          {aboutDescription2.beforeTag}
          <a onClick={handleAnchorClick} href="https://github.com/brave/brave-browser/wiki/Brave-Leo" target="_blank">
            {aboutDescription2.duringTag}
          </a>
          {aboutDescription2.afterTag}
        </p>
      </section>
    </div>
  )
}

export default PrivacyMessage
