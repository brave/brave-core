/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton, SubText } from '../../components'

// Images
import { WelcomeRewardsImage } from '../../components/images'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  index: number
  currentScreen: number
  walletCreated: boolean
  walletCreating: boolean
  walletCreateFailed: boolean
  onClick: () => void
  onWalletCreated: () => void
}

interface State {
  turnedOn: boolean
}

export default class PaymentsBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      turnedOn: false
    }
  }

  componentDidUpdate (prevProps: Props) {
    if (!prevProps.walletCreated && this.props.walletCreated) {
      this.props.onWalletCreated()
    }
  }

  getButtonText = () => {
    const { walletCreating, walletCreateFailed } = this.props

    if (walletCreateFailed) {
      return getLocale('walletFailedButton')
    }

    if (walletCreating || this.state.turnedOn) {
      return getLocale('turningOnRewards')
    }

    return getLocale('turnOnRewards')
  }

  turnOnRewards = () => {
    this.setState({ turnedOn: true })
    this.props.onClick()
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
        <WelcomeRewardsImage />
        <Title>{getLocale('turnOnRewards')}</Title>
        <Paragraph>{getLocale('setupBraveRewards')}</Paragraph>
        <PrimaryButton
          level='primary'
          type='accent'
          size='large'
          text={this.getButtonText()}
          onClick={this.turnOnRewards}
        />
        <SubText>
          {getLocale('serviceText')} <a href={'https://brave.com/terms-of-use'}>{getLocale('termsOfService')}</a> {getLocale('and')} <a href={'https://brave.com/privacy#rewards'}>{getLocale('privacyPolicy')}</a>.
        </SubText>
      </Content>
    )
  }
}
