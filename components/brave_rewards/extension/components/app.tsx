/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { PanelWelcome } from 'brave-ui/features/rewards'

// Components
import WalletPanel from './walletPanel'

// Constants
import { ApplicationState, ComponentProps } from '../constants/rewardsPanelState'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'

import { getUIMessages } from '../background/api/locale_api'

interface Props extends ComponentProps {
}

export class RewardsPanel extends React.Component<Props, {}> {

  render () {
    const { rewardsPanelData } = this.props

    return (
      <>
        {
          !rewardsPanelData.walletCreated
          ? <PanelWelcome
            variant={'one'}
            optInAction={getUIMessages}
          />
          : <WalletPanel />
        }
      </>
    )
  }
}

export const mapStateToProps = (state: ApplicationState) => ({
  rewardsPanelData: state.rewardsPanelData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsPanel)
