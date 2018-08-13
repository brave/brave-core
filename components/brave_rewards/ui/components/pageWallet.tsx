/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import BigNumber from 'bignumber.js'

// Components
import {
  ModalActivity,
  ModalBackupRestore,
  WalletWrapper,
  WalletEmpty
} from 'brave-ui/features/rewards'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import Grant from './grant'
import WalletOff from '../../../../node_modules/brave-ui/features/rewards/walletOff'

// Assets
const walletIcon = require('../../../img/rewards/wallet_icon.svg')
const fundsIcon = require('../../../img/rewards/funds_icon.svg')

interface State {
  modalBackup: boolean,
  modalBackupActive: 'backup' | 'restore'
  modalActivity: boolean
}

interface Props extends Rewards.ComponentProps {
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalBackup: false,
      modalBackupActive: 'backup',
      modalActivity: false
    }
  }

  get actions () {
    return this.props.actions
  }

  onModalBackupToggle = () => {
    if (!this.state.modalBackup && this.props.rewardsData.recoveryKey.length === 0) {
      this.actions.getWalletPassphrase()
    }

    if (this.state.modalBackup) {
      this.actions.onModalBackupClose()
    }

    this.setState({
      modalBackup: !this.state.modalBackup
    })
  }

  onModalBackupTabChange = (tabId: 'backup' | 'restore') => {
    this.setState({
      modalBackupActive: tabId
    })
  }

  onModalBackupOnCopy = () => {
    // TODO NZ implement
    console.log('onModalBackupOnCopy')
  }

  onModalBackupOnPrint = () => {
    // TODO NZ implement
    console.log('onModalBackupOnPrint')
  }

  onModalBackupOnSaveFile = () => {
    // TODO NZ implement
    console.log('onModalBackupOnSaveFile')
  }

  onModalBackupOnRestore = (key: string | MouseEvent) => {
    if (typeof key === 'string' && key.length > 0) {
      this.actions.recoverWallet(key)
    }
  }

  onModalBackupOnImport = () => {
    // TODO NZ implement
    console.log('onModalBackupOnImport')
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

  onModalActivityToggle = () => {
    this.setState({
      modalActivity: !this.state.modalActivity
    })
  }

  onModalActivityAction (action: string) {
    // TODO NZ implement
    console.log(action)
  }

  onModalActivityRemove () {
    // TODO NZ implement
    console.log('onModalActivityRemove')
  }

  render () {
    const { connectedWallet, recoveryKey, promotion, enabledMain } = this.props.rewardsData
    const { balance } = this.props.rewardsData.walletInfo
    const { walletRecoverySuccess, emptyWallet } = this.props.rewardsData.ui

    return (
      <>
        {
          enabledMain && promotion && promotion.promotionId
          ? <Grant/>
          : null
        }
        <WalletWrapper
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
            enabledMain
            ? emptyWallet
              ? <WalletEmpty />
              : null // TODO NZ <PanelSummary grant={} ads={} contribute={} donation={} tips={} onActivity={} />
            : <WalletOff/>
          }
        </WalletWrapper>
        {
          this.state.modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.modalBackupActive}
              backupKey={recoveryKey}
              onTabChange={this.onModalBackupTabChange}
              onClose={this.onModalBackupToggle}
              onCopy={this.onModalBackupOnCopy}
              onPrint={this.onModalBackupOnPrint}
              onSaveFile={this.onModalBackupOnSaveFile}
              onRestore={this.onModalBackupOnRestore}
              success={walletRecoverySuccess === true ? getLocale('walletRecoverySuccess', { balance: balance.toString() }) : ''}
              error={walletRecoverySuccess === false ? getLocale('walletRecoveryFail') : ''}
            />
            : null
        }
        {
          // TODO NZ add actual data for the whole section
          this.state.modalActivity
            ? <ModalActivity
              contributeRows={[
                {
                  profile: {
                    name: 'Bart Baker',
                    verified: true,
                    provider: 'youtube',
                    src: ''
                  },
                  attention: 40,
                  onRemove: this.onModalActivityRemove,
                  token: {
                    value: 5,
                    converted: 5
                  }
                }
              ]}
              transactionRows={[
                {
                  date: '6/1',
                  type: 'deposit',
                  description: 'Brave Ads payment for May',
                  amount: {
                    value: 5,
                    converted: 5
                  }
                }
              ]}
              onClose={this.onModalActivityToggle}
              onPrint={this.onModalActivityAction.bind('onPrint')}
              onDownloadPDF={this.onModalActivityAction.bind('onDownloadPDF')}
              onMonthChange={this.onModalActivityAction.bind('onMonthChange')}
              months={{
                'aug-2018': 'August 2018',
                'jul-2018': 'July 2018',
                'jun-2018': 'June 2018',
                'may-2018': 'May 2018',
                'apr-2018': 'April 2018'
              }}
              currentMonth={'aug-2018'}
              summary={[
                {
                  text: 'Token Grant available',
                  type: 'grant',
                  token: {
                    value: 10,
                    converted: 5.20
                  }
                },
                {
                  text: 'Earnings from Brave Ads',
                  type: 'ads',
                  token: {
                    value: 10,
                    converted: 5.20
                  }
                },
                {
                  text: 'Brave Contribute',
                  type: 'contribute',
                  notPaid: true,
                  token: {
                    value: 10,
                    converted: 5.20,
                    isNegative: true
                  }
                },
                {
                  text: 'Recurring Donations',
                  type: 'recurring',
                  notPaid: true,
                  token: {
                    value: 2,
                    converted: 1.1,
                    isNegative: true
                  }
                },
                {
                  text: 'One-time Donations/Tips',
                  type: 'donations',
                  token: {
                    value: 19,
                    converted: 10.10,
                    isNegative: true
                  }
                }
              ]}
              total={{
                value: 1,
                converted: 0.5
              }}
              paymentDay={12}
              openBalance={{
                value: 10,
                converted: 5.20
              }}
              closingBalance={{
                value: 11,
                converted: 5.30
              }}
            />
            : null
        }
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
