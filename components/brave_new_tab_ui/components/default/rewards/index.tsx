/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import { convertBalance } from '../../../../brave_rewards/resources/page/utils'
import { getLocale } from '../../../../common/locale'

import {
  WidgetWrapper,
  BatIcon,
  RewardsTitle,
  Footer,
  ServiceText,
  ServiceLink,
  LearnMoreText,
  PreOptInInfo,
  Title,
  SubTitle,
  PreOptInAction,
  TurnOnButton,
  AmountItem,
  AmountInformation,
  AmountDescription,
  Amount,
  ConvertedAmount,
  LearnMoreLink,
  TurnOnAdsButton,
  UnsupportedMessage
} from './style'
import Notification from './notification'
import { BatColorIcon } from 'brave-ui/components/icons'

export interface RewardsProps {
  enabledAds: boolean
  enabledMain: boolean
  balance: NewTab.RewardsBalance
  grants: NewTab.GrantRecord[]
  totalContribution: string
  walletCreated: boolean
  walletCreating: boolean
  walletCreateFailed: boolean
  walletCorrupted: boolean
  adsEstimatedEarnings: number
  onlyAnonWallet?: boolean
  adsSupported?: boolean
  onCreateWallet: () => void
  onEnableAds: () => void
  onEnableRewards: () => void
  onDismissNotification: (id: string) => void
}

const enum AmountItemType {
  ADS = 0,
  TIPS = 1
}

class Rewards extends React.PureComponent<RewardsProps, {}> {

  getButtonText = (isAds: boolean = false) => {
    if (isAds) {
      return getLocale('rewardsWidgetTurnOnAds')
    }

    const {
      walletCreating,
      walletCreateFailed,
      walletCorrupted
    } = this.props

    if (walletCreateFailed || walletCorrupted) {
      return getLocale('rewardsWidgetWalletFailedButton')
    }

    if (walletCreating) {
      return getLocale('rewardsWidgetTurningOn')
    }

    return getLocale('rewardsWidgetTurnOnRewards')
  }

  optInAction = (hasEnabled: boolean) => {
    if (hasEnabled) {
      this.props.onEnableRewards()
    } else {
      this.props.onCreateWallet()
    }
  }

  renderPreOptIn = () => {
    const {
      enabledMain,
      walletCreated
    } = this.props

    if (enabledMain && walletCreated) {
      return null
    }

    const hasEnabled = !enabledMain && walletCreated

    return (
      <>
        <PreOptInInfo>
          <Title>
            {
              hasEnabled
              ? getLocale('rewardsWidgetReEnableTitle')
              : getLocale('rewardsWidgetEnableTitle')
            }
          </Title>
          <SubTitle>
            {
              hasEnabled
              ? getLocale('rewardsWidgetReEnableSubTitle')
              : getLocale('rewardsWidgetEnableSubTitle')
            }
          </SubTitle>
        </PreOptInInfo>
        <PreOptInAction>
          <TurnOnButton
            data-test-id={'optInAction'}
            onClick={this.optInAction.bind(this, hasEnabled)}
          >
            {this.getButtonText()}
          </TurnOnButton>
        </PreOptInAction>
      </>
    )
  }

  renderAmountItem = (type: AmountItemType) => {
    const {
      balance,
      enabledAds,
      onEnableAds,
      adsEstimatedEarnings,
      onlyAnonWallet,
      totalContribution,
      adsSupported
    } = this.props

    const rates = balance.rates || {}
    const showEnableAds = type === AmountItemType.ADS && !enabledAds && adsSupported
    const amount = type === AmountItemType.TIPS
      ? totalContribution
      : adsEstimatedEarnings.toFixed(1)
    const converted = convertBalance(amount, rates)
    const batFormatString = onlyAnonWallet ? getLocale('rewardsWidgetBap') : getLocale('rewardsWidgetBat')

    return (
      <AmountItem isLast={type === AmountItemType.TIPS}>
        <AmountDescription>
          {
            type === AmountItemType.ADS
            ? getLocale('rewardsWidgetEstimatedEarnings')
            : getLocale('rewardsWidgetMonthlyTips')
          }
        </AmountDescription>
        {
          showEnableAds
          ? <TurnOnAdsButton onClick={onEnableAds}>
              {this.getButtonText(true)}
            </TurnOnAdsButton>
          : null
        }
        {
          !showEnableAds && !(type === AmountItemType.ADS && !adsSupported)
          ? <AmountInformation data-test-id={`widget-amount-total-${type}`}>
              <Amount>{amount}</Amount>
              <ConvertedAmount>
                {`${batFormatString} ${converted} USD`}
              </ConvertedAmount>
            </AmountInformation>
          : null
        }
        {
          type === AmountItemType.ADS && !adsSupported
          ? <UnsupportedMessage>
              {getLocale('rewardsWidgetAdsNotSupported')}
            </UnsupportedMessage>
          : null
        }
      </AmountItem>
    )
  }

  renderRewardsInfo = () => {
    const {
      enabledMain,
      walletCreated
    } = this.props

    if (!enabledMain || !walletCreated) {
      return null
    }

    return (
      <div data-test-id2={'enableMain'}>
        {this.renderAmountItem(AmountItemType.ADS)}
        {this.renderAmountItem(AmountItemType.TIPS)}
      </div>
    )
  }

  renderLearnMore = () => {
    return (
      <LearnMoreText>
        <LearnMoreLink target={'_blank'} href={'chrome://rewards'}>
          {getLocale('learnMore')}
        </LearnMoreLink>
        {getLocale('rewardsWidgetAboutRewards')}
      </LearnMoreText>
    )
  }

  renderPrivacyPolicy = () => {
    return (
      <>
        <ServiceText>
          {getLocale('rewardsWidgetServiceText')} <ServiceLink target={'_blank'} href={'https://brave.com/terms-of-use'}>{getLocale('rewardsWidgetTermsOfService')}</ServiceLink> {getLocale('rewardsWidgetAnd')} <ServiceLink target={'_blank'} href={'https://brave.com/privacy#rewards'}>{getLocale('rewardsWidgetPrivacyPolicy')}</ServiceLink>.
        </ServiceText>
      </>
    )
  }

  renderNotifications = () => {
    const { grants, onDismissNotification } = this.props

    return (
      <>
        {grants.map((grant: NewTab.GrantRecord, index) => {
          return (
            <Notification
              grant={grant}
              key={`notification-${index}`}
              onDismissNotification={onDismissNotification}
            />
          )
        })}
      </>
    )
  }

  render () {
    const {
      enabledMain,
      walletCreated
    } = this.props

    return (
      <WidgetWrapper>
        <BatIcon>
          <BatColorIcon />
        </BatIcon>
        <RewardsTitle>
          {getLocale('rewardsWidgetBraveRewards')}
        </RewardsTitle>
        {this.renderPreOptIn()}
        {this.renderRewardsInfo()}
        <Footer>
          {
            enabledMain && walletCreated
            ? this.renderLearnMore()
            : this.renderPrivacyPolicy()
          }
        </Footer>
        {
          enabledMain
          ? this.renderNotifications()
          : null
        }
      </WidgetWrapper>
    )
  }
}

export const RewardsWidget = createWidget(Rewards)
