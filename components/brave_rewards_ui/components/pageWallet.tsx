/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  ModalBackupRestore,
  Panel,
  PanelEmpty
} from 'brave-ui/features/rewards'
import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import BigNumber from 'bignumber.js'

// Utils
import { getLocale } from '../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
// Assets
const walletIcon = require('../../img/rewards/wallet_icon.svg')
const fundsIcon = require('../../img/rewards/funds_icon.svg')

interface State {
  modalBackup: boolean,
  modalBackupActive: 'backup' | 'restore'
}

interface Props extends Rewards.ComponentProps {
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalBackup: false,
      modalBackupActive: 'backup'
    }
  }

  onModalBackupToggle = () => {
    this.setState({
      modalBackup: !this.state.modalBackup
    })
  }

  onModalBackupTabChange = (tabId: 'backup' | 'restore') => {
    this.setState({
      modalBackupActive: tabId
    })
  }

  onModalBackupAction (action: string) {
    // TODO NZ implement
    console.log(action)
  }

  getConversion = () => {
    const walletInfo = this.props.rewardsData.walletInfo
    return utils.convertBalance(walletInfo.balance, walletInfo.rates)
  }

  getGrants = () => {
    const grants = this.props.rewardsData.walletInfo.grants
    if (!grants) {
      return []
    }

    return grants.map((grant: Rewards.Grant) => {
      return {
        tokens: new BigNumber(grant.probi.toString()).dividedBy('1e18').toNumber(),
        expireDate: new Date(grant.expiryTime * 1000).toLocaleDateString()
      }
    })
  }

  render () {
    const { connectedWallet, recoveryKey, wasFunded } = this.props.rewardsData
    const { balance } = this.props.rewardsData.walletInfo

    return (
      <>
        {
          this.state.modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.modalBackupActive}
              recoveryKey={recoveryKey}
              onTabChange={this.onModalBackupTabChange}
              onClose={this.onModalBackupToggle}
              onCopy={this.onModalBackupAction.bind('onCopy')}
              onPrint={this.onModalBackupAction.bind('onPrint')}
              onSaveFile={this.onModalBackupAction.bind('onSaveFile')}
              onRestore={this.onModalBackupAction.bind('onRestore')}
              onImport={this.onModalBackupAction.bind('onImport')}
            />
            : null
        }
        <Panel
          tokens={balance}
          converted={utils.formatConverted(this.getConversion())}
          actions={[
            {
              name: getLocale('panelAddFunds'),
              action: () => { console.log('panelAddFunds') },
              icon: walletIcon
            },
            {
              name: getLocale('panelWithdrawFunds'),
              action: () => { console.log('panelWithdrawFunds') },
              icon: fundsIcon
            }
          ]}
          onSettingsClick={this.onModalBackupToggle}
          showCopy={true}
          showSecActions={true}
          grants={this.getGrants()}
          connectedWallet={connectedWallet}
        >
          {
            wasFunded
            ? null // TODO NZ <PanelSummary grant={} ads={} contribute={} donation={} tips={} onActivity={} />
            : <PanelEmpty />
          }
        </Panel>
      </>
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
)(PageWallet)
