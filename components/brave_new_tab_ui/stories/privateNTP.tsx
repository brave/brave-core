/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { boolean } from '@storybook/addon-knobs'

// Components
import NewPrivateTab from './privateNTP/index'

export default {
  title: 'New Tab/PrivateNTP'
}

export const PrivateWindow = () => {
  return (
    <NewPrivateTab isQwant={boolean('Is Qwant?', false)} isTor={boolean('Enable Tor?', false)} />
  )
}

export const QwantWindow = () => {
  return (
    <NewPrivateTab isQwant={boolean('Is Qwant?', true)} isTor={boolean('Enable Tor?', false)} />
  )
}

export const QwantTor = () => {
  return (
    <NewPrivateTab isQwant={boolean('Is Qwant?', true)} isTor={boolean('Enable Tor?', true)} />
  )
}

export const TorWindow = () => {
  return (
    <NewPrivateTab isQwant={boolean('Is Qwant?', false)} isTor={boolean('Enable Tor?', true)} />
  )
}
