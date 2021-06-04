/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import BoxMobile, { Props as BoxMobileProps } from '../../ui/components/mobile/boxMobile'
import { List, NextContribution, Tokens } from '../../ui/components'
import { Grid, Column, Select, ControlWrapper } from 'brave-ui/components'
import AdsOnboarding from './adsOnboarding'
import {
  StyledArrivingSoon,
  StyledListContent,
  StyledTotalContent
} from './style'

import { MoneyBagIcon } from '../../shared/components/icons/money_bag'
import { formatMessage } from '../../shared/lib/locale_context'
import { getDaysUntilRewardsPayment } from '../../shared/lib/pending_rewards'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

const nextPaymentDateFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric'
})

interface Props extends Rewards.ComponentProps {
}

class AdsBox extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  getAdsSubdivisions = () => {
    const {
      adsSubdivisionTargeting,
      automaticallyDetectedAdsSubdivisionTargeting
    } = this.props.rewardsData.adsData

    let subdivisions: any = [
      ['US-AL', 'Alabama'],
      ['US-AK', 'Alaska'],
      ['US-AZ', 'Arizona'],
      ['US-AR', 'Arkansas'],
      ['US-CA', 'California'],
      ['US-CO', 'Colorado'],
      ['US-CT', 'Connecticut'],
      ['US-DE', 'Delaware'],
      ['US-FL', 'Florida'],
      ['US-GA', 'Georgia'],
      ['US-HI', 'Hawaii'],
      ['US-ID', 'Idaho'],
      ['US-IL', 'Illinois'],
      ['US-IN', 'Indiana'],
      ['US-IA', 'Iowa'],
      ['US-KS', 'Kansas'],
      ['US-KY', 'Kentucky'],
      ['US-LA', 'Louisiana'],
      ['US-ME', 'Maine'],
      ['US-MD', 'Maryland'],
      ['US-MA', 'Massachusetts'],
      ['US-MI', 'Michigan'],
      ['US-MN', 'Minnesota'],
      ['US-MS', 'Mississippi'],
      ['US-MO', 'Missouri'],
      ['US-MT', 'Montana'],
      ['US-NE', 'Nebraska'],
      ['US-NV', 'Nevada'],
      ['US-NH', 'New Hampshire'],
      ['US-NJ', 'New Jersey'],
      ['US-NM', 'New Mexico'],
      ['US-NY', 'New York'],
      ['US-NC', 'North Carolina'],
      ['US-ND', 'North Dakota'],
      ['US-OH', 'Ohio'],
      ['US-OK', 'Oklahoma'],
      ['US-OR', 'Oregon'],
      ['US-PA', 'Pennsylvania'],
      ['US-RI', 'Rhode Island'],
      ['US-SC', 'South Carolina'],
      ['US-SD', 'South Dakota'],
      ['US-TN', 'Tennessee'],
      ['US-TX', 'Texas'],
      ['US-UT', 'Utah'],
      ['US-VT', 'Vermont'],
      ['US-VA', 'Virginia'],
      ['US-WA', 'Washington'],
      ['US-WV', 'West Virginia'],
      ['US-WI', 'Wisconsin'],
      ['US-WY', 'Wyoming']
    ]

    if (adsSubdivisionTargeting === 'DISABLED') {
      subdivisions.unshift(['DISABLED', getLocale('adsSubdivisionTargetingDisabled')])
    } else {
      subdivisions.unshift(['DISABLED', getLocale('adsSubdivisionTargetingDisable')])
    }

    const subdivisionMap = new Map(subdivisions)
    const subdivision = subdivisionMap.get(automaticallyDetectedAdsSubdivisionTargeting) as string
    if (subdivision !== '' && adsSubdivisionTargeting === 'AUTO') {
      subdivisions.unshift(['AUTO', getLocale('adsSubdivisionTargetingAutoDetectedAs', { adsSubdivisionTarget : subdivision })])
    } else {
      subdivisions.unshift(['AUTO', getLocale('adsSubdivisionTargetingAutoDetect')])
    }

    return subdivisions
  }

  onAdsSettingChange = (key: string, value: boolean) => {
    const { adsEnabled } = this.props.rewardsData.adsData

    let newValue: any = value

    if (key === 'adsEnabled') {
      newValue = !adsEnabled
    }

    this.props.actions.onAdsSettingSave(key, newValue)
  }

  adsSettings = (enabled?: boolean) => {
    if (!enabled) {
      return null
    }

    const {
      adsPerHour,
      shouldAllowAdsSubdivisionTargeting,
      adsSubdivisionTargeting
    } = this.props.rewardsData.adsData

    return (
      <Grid columns={1} customStyle={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('adsPerHour')}>
            <Select
              value={adsPerHour.toString()}
              onChange={this.onAdsSettingChange.bind(this, 'adsPerHour')}
            >
              {['0', '1', '2', '3', '4', '5', '10'].map((num: string) => {
                return (
                  <div key={`num-per-hour-${num}`} data-value={num}>
                    {getLocale(`adsPerHour${num}`)}
                  </div>
                )
              })}
            </Select>
          </ControlWrapper>
        </Column>
        { shouldAllowAdsSubdivisionTargeting ?
          <>
            <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
              <ControlWrapper text={getLocale('adsSubdivisionTargetingTitle')}>
                <Select
                  value={(adsSubdivisionTargeting || '').toString()}
                  onChange={this.onAdsSettingChange.bind(this, 'adsSubdivisionTargeting')}
                >
                  {
                    this.getAdsSubdivisions().map((subdivision: Array<String>) => {
                      return (
                        <div key={`${subdivision[0]}`} data-value={subdivision[0]}>
                          {`${subdivision[1]}`}
                        </div>
                      )
                    })
                  }
                </Select>
              </ControlWrapper>
            </Column>
            <div>
              {getLocale('adsSubdivisionTargetingDescription')} <a href={'https://support.brave.com/hc/en-us/articles/360026361072-Brave-Ads-FAQ'} target={'_blank'}>{getLocale('adsSubdivisionTargetingLearn')}</a>
            </div>
          </> : null }
      </Grid>
    )
  }

  render () {
    // Default values from storage.ts
    let adsEnabled = false
    let adsUIEnabled = false
    let adsIsSupported = false
    let nextPaymentDate = 0
    let adsReceivedThisMonth = 0
    let earningsThisMonth = 0
    let earningsLastMonth = 0

    const {
      adsData,
      safetyNetFailed,
      parameters
    } = this.props.rewardsData

    if (adsData) {
      adsEnabled = adsData.adsEnabled
      adsUIEnabled = adsData.adsUIEnabled
      adsIsSupported = adsData.adsIsSupported
      nextPaymentDate = adsData.adsNextPaymentDate
      adsReceivedThisMonth = adsData.adsReceivedThisMonth || 0
      earningsThisMonth = adsData.adsEarningsThisMonth || 0
      earningsLastMonth = adsData.adsEarningsLastMonth || 0
    }

    // disabled / alert state
    const isDisabled = safetyNetFailed || !adsIsSupported || !adsUIEnabled
    const toggle = !isDisabled
    // Sanity-check: ensure no action can be performed if the box isn't allowed
    // to be enabled
    const toggleAction = toggle
      ? this.onAdsSettingChange.bind(this, 'adsEnabled', '')
      : () => undefined
    const boxPropsExtra: Partial<BoxMobileProps> = {
      toggle,
      toggleAction,
      checked: toggle && adsEnabled
    }
    if (isDisabled) {
      boxPropsExtra.alertContent = safetyNetFailed
         ? <>{getLocale('adsNotSupportedDevice')}</>
         : !adsIsSupported
           ? <>{getLocale('adsNotSupportedRegion')}</>
           : <>This version of Brave does not support Ads.</>
    }
    if (!isDisabled && !boxPropsExtra.checked) {
      boxPropsExtra.extraDescriptionChild = <AdsOnboarding />
    }

    const tokenString = getLocale('tokens')
    const estimatedPendingDays = getDaysUntilRewardsPayment(nextPaymentDate)

    return (
      <BoxMobile
        title={getLocale('adsTitle')}
        type={'ads'}
        description={getLocale('adsDesc', { currency : tokenString })}
        settingsChild={this.adsSettings(adsEnabled)}
        {...boxPropsExtra}
      >
        {
          earningsLastMonth > 0 && estimatedPendingDays &&
            <StyledArrivingSoon>
              <MoneyBagIcon />
              {
                formatMessage(getLocale('pendingRewardsMessage'), [
                  <span className='amount' key='amount'>
                    +{earningsLastMonth} BAT
                  </span>,
                  estimatedPendingDays
                ])
              }
            </StyledArrivingSoon>
        }
        <List title={<StyledListContent>{getLocale('adsCurrentEarnings')}</StyledListContent>}>
          <StyledTotalContent>
            <Tokens
              value={earningsThisMonth.toFixed(3)}
              converted={utils.convertBalance(earningsThisMonth, parameters.rate)}
            />
          </StyledTotalContent>
        </List>
        <List title={<StyledListContent>{getLocale('adsPaymentDate')}</StyledListContent>}>
          <StyledListContent>
            <NextContribution>
              {nextPaymentDateFormatter.format(new Date(nextPaymentDate))}
            </NextContribution>
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{getLocale('adsNotificationsReceived')}</StyledListContent>}>
          <StyledListContent>
            <Tokens
              value={adsReceivedThisMonth.toString()}
              hideText={true}
            />
          </StyledListContent>
        </List>
      </BoxMobile>
    )
  }
}

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(AdsBox)
