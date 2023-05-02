/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import styles from './style.module.scss'
import privacyGraphicUrl from '../../assets/privacy_graphic.svg'
import { getLocale } from '$web-common/locale'

function PrivacyMessage () {
  return (
    <div className={styles.box}>
      <figure>
        <img src={privacyGraphicUrl}
          alt="Illustration: window and chat bubble, representing communication" />
      </figure>
      <section>
        <b>{getLocale('aboutTitle')}</b>
        <p>{getLocale('aboutDescription')}</p>
        <p>{getLocale('aboutDescription_2')}</p>
        <p>{getLocale('aboutNote')}</p>
      </section>
    </div>
  )
}

export default PrivacyMessage
