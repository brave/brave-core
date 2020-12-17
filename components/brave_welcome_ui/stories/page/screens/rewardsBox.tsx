/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, TermsOfService, PrimaryButton } from '../../../components'

// Utils
import locale from '../fakeLocale'

// Images
import { WelcomeRewardsImage } from '../../../components/images'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

export default class PaymentsBox extends React.PureComponent<Props, {}> {
  render () {
    const { index, currentScreen, onClick } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeRewardsImage />
        <Title>{locale.enableBraveRewards}</Title>
        <Paragraph>
          <strong>Earn tokens</strong> by viewing Brave Private Ads and support
          content creators automatically.
        </Paragraph>
        <TermsOfService>
          By clicking, you agree to the <a href='javascript:void 0'>Terms of Service</a>&nbsp;
          and <a href='javascript:void 0'>Privacy Policy</a>.<br />
          You can turn this off at any time in settings.
        </TermsOfService>
        <PrimaryButton
          level='primary'
          type='accent'
          size='large'
          text={locale.getStarted}
          onClick={onClick}
        />
      </Content>
    )
  }
}
