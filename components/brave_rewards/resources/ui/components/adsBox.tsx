/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  Box,
  DisabledContent,
} from 'brave-ui/features/rewards'
import { Grid, Column, Select, ControlWrapper } from 'brave-ui/components'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

// Assets
const adsDisabledIcon = require('../../img/ads_disabled.svg')

interface Props extends Rewards.ComponentProps {
}

class AdsBox extends React.Component<Props, {}> {

  adsDisabled = () => {
    return (
      <DisabledContent
        image={adsDisabledIcon}
        type={'ads'}
      >
        <h3>{getLocale('adsDisabledText')}</h3>
      </DisabledContent>
    )
  }

  onAdsSettingChange = (key: string, value: string) => {
    let newValue: any = value
    const { adsEnabled } = this.props.rewardsData.adsData

    if (key === 'adsEnabled') {
      newValue = !adsEnabled
    }

    if (key === 'adsPerHour') {
      newValue = +value
    }

    this.props.actions.onAdsSettingSave(key, newValue)
  }

  adsSettings = (enabled?: boolean) => {
    if (!enabled) {
      return null
    }

    const { adsPerHour } = this.props.rewardsData.adsData

    return (
      <Grid columns={1} customStyle={{ maxWidth: '270px', margin: '0 auto' }}>
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

  render () {
    let adsEnabled
    const { adsData } = this.props.rewardsData

    if (!adsData) {
      adsEnabled = false
    } else {
      adsEnabled = this.props.rewardsData.adsData.adsEnabled
    }

    return (
      <Box
        title={getLocale('adsTitle')}
        type={'ads'}
        description={getLocale('adsDesc')}
        toggle={true}
        checked={adsEnabled}
        settingsChild={this.adsSettings(adsEnabled)}
        testId={'braveAdsSettings'}
        disabledContent={this.adsDisabled()}
        onToggle={this.onAdsSettingChange.bind(this, 'adsEnabled', '')}
      />
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
