/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  Box,
  BoxAlert,
  DisabledContent,
  List,
  NextContribution,
  Tokens
} from 'brave-ui/features/rewards'
import { Grid, Column, Select, ControlWrapper } from 'brave-ui/components'

// Utils
import * as utils from '../utils'
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

interface Props extends Rewards.ComponentProps {
}

interface State {
  settings: boolean
}

class AdsBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      settings: false
    }
  }

  componentDidUpdate (prevProps: Props) {
    if (
      prevProps.rewardsData.enabledMain &&
      !this.props.rewardsData.enabledMain
    ) {
      this.setState({ settings: false })
    }
  }

  adsDisabled = () => {
    return (
      <DisabledContent
        type={'ads'}
      >
        {getLocale('adsDisabledTextOne')} <br />
        {getLocale('adsDisabledTextTwo')}
      </DisabledContent>
    )
  }

  adsNotSupportedAlert = (supported: boolean) => {
    if (supported) {
      return null
    }

    return (
      <BoxAlert type={'ads'} />
    )
  }

  onAdsSettingChange = (key: string, value: string) => {
    let newValue: any = value
    const { adsEnabled } = this.props.rewardsData.adsData

    if (key === 'adsEnabled') {
      newValue = !adsEnabled
    }

    this.props.actions.onAdsSettingSave(key, newValue)
  }

  adsSettings = (enabled?: boolean) => {
    if (!enabled) {
      return null
    }

    const { adsPerHour } = this.props.rewardsData.adsData

    return (
      <Grid columns={1} customStyle={{ margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('adsPerHour')}>
            <Select
              value={adsPerHour.toString()}
              onChange={this.onAdsSettingChange.bind(this, 'adsPerHour')}
            >
              {['1', '2', '3', '4', '5'].map((num: string) => {
                return (
                  <div key={`num-per-hour-${num}`} data-value={num}>
                   {getLocale(`adsPerHour${num}`)}
                  </div>
                )
              })}
            </Select>
          </ControlWrapper>
        </Column>
      </Grid>
    )
  }

  onSettingsToggle = () => {
    this.setState({ settings: !this.state.settings })
  }

  render () {
    // Default values from storage.ts
    let adsEnabled = false
    let adsUIEnabled = false
    let notificationsReceived = 0
    let estimatedEarnings = '0'
    let adsIsSupported = false

    const {
      adsData,
      enabledMain,
      firstLoad,
      walletInfo
    } = this.props.rewardsData

    if (adsData) {
      adsEnabled = adsData.adsEnabled
      adsUIEnabled = adsData.adsUIEnabled
      notificationsReceived = adsData.adsNotificationsReceived || 0
      estimatedEarnings = (adsData.adsEstimatedEarnings || 0).toFixed(2)
      adsIsSupported = adsData.adsIsSupported
    }

    const enabled = adsEnabled && adsIsSupported
    const toggle = !(!enabledMain || !adsUIEnabled || !adsIsSupported)
    const showDisabled = firstLoad !== false || !toggle || !adsEnabled || !adsIsSupported

    return (
      <Box
        title={getLocale('adsTitle')}
        type={'ads'}
        description={getLocale('adsDesc')}
        toggle={toggle}
        checked={enabled}
        settingsChild={this.adsSettings(enabled && enabledMain)}
        testId={'braveAdsSettings'}
        disabledContent={showDisabled ? this.adsDisabled() : null}
        onToggle={this.onAdsSettingChange.bind(this, 'adsEnabled', '')}
        settingsOpened={this.state.settings}
        onSettingsClick={this.onSettingsToggle}
        attachedAlert={this.adsNotSupportedAlert(adsIsSupported)}
      >
        <List title={getLocale('adsCurrentEarnings')}>
          <Tokens
            value={estimatedEarnings}
            converted={utils.convertBalance(estimatedEarnings, walletInfo.rates)}
          />
        </List>
        <List title={getLocale('adsPaymentDate')}>
          <NextContribution>
            {'Monthly, 5th'}
          </NextContribution>
        </List>
        <List title={getLocale('adsNotificationsReceived')}>
          <Tokens
            value={notificationsReceived.toString()}
            hideText={true}
          />
        </List>
      </Box>
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
