/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  ModalBackupRestore,
  Panel,
  PanelEmpty
} from 'brave-ui'
import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Utils
import { getLocale } from '../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

// Assets
const walletIcon = require('../../img/rewards/wallet_icon.svg')
const fundsIcon = require('../../img/rewards/funds_icon.svg')

interface State {
  modalBackup: boolean,
  modalBackupActive: 'backup' | 'restore'
}

class PageWallet extends React.Component<{}, State> {
  constructor (props: {}) {
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
    console.log(action)
  }

  render () {
    return (
      <>
        {
          this.state.modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.modalBackupActive}
              recoveryKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
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
          tokens={25}
          converted={'6.0 USD'}
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
          grants={[
            {
              tokens: 8,
              expireDate: '7/15/2018'
            },
            {
              tokens: 10,
              expireDate: '9/10/2018'
            },
            {
              tokens: 10,
              expireDate: '10/10/2018'
            }
          ]}
          connectedWallet={false}
        >
          <PanelEmpty />
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
