// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
// Feature-specific components
import {
  Content,
  Title,
  SubTitle,
  Text,
  TorLockImage
} from '../../components/private'
// Helpers
import { getLocale } from '../../../common/locale'

const isMac = window.navigator.userAgent.includes('Macintosh')
const torKeyboardShortcutText = isMac ? '⌥⌘N' : 'Alt+Shift+N'

export default function TorContent () {
  return (
    <Content>
      <TorLockImage />
      <SubTitle>{getLocale('boxTorLabel')}</SubTitle>
      <Title>{getLocale('boxTorTitle')}</Title>
      <Text>{getLocale('boxTorText2', { key: torKeyboardShortcutText })}</Text>
    </Content>
  )
}
