/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, TermsOfService, PrimaryButton } from '../../components'

// Images
import { WelcomeRewardsImage } from '../../components/images'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

function splitMessage (key: string) {
  return getLocale(key).split(/\$\d+/g)
}

export default class PaymentsBox extends React.PureComponent<Props, {}> {
  renderText () {
    const [
      before,
      during,
      after
    ] = splitMessage('setupBraveRewards')

    return <>{before}<strong>{during}</strong>{after}</>
  }

  renderTerms () {
    const [
      beforeLink1,
      link1,
      afterLink1,
      link2,
      afterLink2
    ] = splitMessage('braveRewardsTerms')

    return (
      <>
        {beforeLink1}
        <a
          href='https://basicattentiontoken.org/user-terms-of-service'
          target='_blank'
          rel='noopener noreferrer'
        >
          {link1}
        </a>
        {afterLink1}
        <a
          href='https://brave.com/privacy/#rewards'
          target='_blank'
          rel='noopener noreferrer'
        >
          {link2}
        </a>
        {afterLink2}
        <br />
        {getLocale('braveRewardsNote')}
      </>
    )
  }

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
        <Title>{getLocale('braveRewardsTitle')}</Title>
        <Paragraph>{this.renderText()}</Paragraph>
        <TermsOfService>{this.renderTerms()}</TermsOfService>
        <PrimaryButton
          level='primary'
          type='accent'
          size='large'
          text={getLocale('enableRewards')}
          onClick={onClick}
        />
      </Content>
    )
  }
}
