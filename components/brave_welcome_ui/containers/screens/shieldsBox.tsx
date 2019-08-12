/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, Link } from '../../components'

// Utils
import { getLocale } from '../../../common/locale'

// Images
import { WelcomeP3AImage } from '../../components/images'

// API
import * as tabsAPI from '../../../brave_extension/extension/brave_extension/background/api/tabsAPI'

interface Props {
  index: number
  currentScreen: number
}

export default class ShieldsBox extends React.PureComponent<Props> {
  openSettings = () => {
    tabsAPI.createTab({ url: 'chrome://settings/privacy' })
      .catch((err) => console.log('[Unable to open a new tab from P3A panel]', err))
  }

  render () {
    const { index, currentScreen } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeP3AImage />
        <Title>{getLocale('privacyTitle')}</Title>
        <Paragraph>
          {getLocale('privacyDesc')}
          <Link onClick={this.openSettings}>&nbsp;{getLocale('privacySettings')}</Link>.
        </Paragraph>
      </Content>
    )
  }
}
