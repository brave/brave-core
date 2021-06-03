/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { ExtendedActivityRow as ActivityRows } from '../../components/modalActivity'
import { DetailRow as TransactionsRow } from '../../components/tableTransactions'

import {
  ModalActivity,
  ModalBackupRestore,
  WalletEmpty,
  WalletSummary,
  WalletWrapper
} from '../../components'
import { WalletState } from '../../components/walletWrapper'
import { WalletAddIcon, WalletWithdrawIcon } from 'brave-ui/components/icons'

// Assets
const favicon = require('../img/brave-favicon.png')
const buzz = require('../img/buzz.jpg')
const ddgo = require('../img/ddgo.jpg')
const guardian = require('../img/guardian.jpg')
const wiki = require('../img/wiki.jpg')

export interface Props {
  content: 'empty' | 'summary' | 'off'
  walletState: WalletState
}

interface State {
  activeTabId: number
  modalBackup: boolean
  modalActivity: boolean
}

const doNothing = () => {
  console.log('nothing')
}

class PageWallet extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      activeTabId: 0,
      modalBackup: false,
      modalActivity: false
    }
  }

  get activityContributions (): ActivityRows[] {
    return [
      {
        profile: {
          name: 'Jonathon Doe',
          verified: true,
          provider: 'youtube',
          src: favicon
        },
        url: 'https://brave.com',
        amount: {
          tokens: '5.0',
          converted: '5.00'
        },
        type: 'monthly'
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        url: 'https://brave.com',
        amount: {
          tokens: '4.0',
          converted: '11.00'
        },
        type: 'monthly'
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        url: 'https://brave.com',
        amount: {
          tokens: '3.0',
          converted: '15.00'
        },
        type: 'monthly'
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        url: 'https://brave.com',
        amount: {
          tokens: '2.0',
          converted: '17.00'
        },
        type: 'contribute'
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        url: 'https://brave.com',
        amount: {
          tokens: '1.0',
          converted: '11.00'
        },
        type: 'tip'
      }
    ]
  }

  get activityTransactions (): TransactionsRow[] {
    return [
      {
        date: 1576066103000,
        type: 'ads',
        description: 'Brave Ads payment for May',
        amount: {
          value: '5.0',
          converted: '5.00'
        }
      },
      {
        date: 1576066103000,
        type: 'tip',
        description: {
          publisher: 'Jonathon Doe',
          platform: 'YouTube'
        },
        amount: {
          isNegative: true,
          value: '5.0',
          converted: '11.00'
        }
      },
      {
        date: 1576066103000,
        type: 'grant',
        description: 'Token grant made available or unlocked',
        amount: {
          value: '10.0',
          converted: '15.00'
        }
      },
      {
        date: 1576066103000,
        type: 'monthly',
        description: 'coinmarketcap.com',
        amount: {
          isNegative: true,
          value: '10.0',
          converted: '15.00'
        }
      },
      {
        date: 1576066103000,
        type: 'tip',
        description: {
          publisher: 'BrendanEich',
          platform: 'Twitter'
        },
        amount: {
          isNegative: true,
          value: '1.0',
          converted: '2.00'
        }
      },
      {
        date: 1576066103000,
        type: 'contribute',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: '10.0',
          converted: '15.00'
        }
      }
    ]
  }

  onBackupTabChange = () => {
    const newId = this.state.activeTabId === 0 ? 1 : 0
    this.setState({ activeTabId: newId })
  }

  onBackupModalClose = () => {
    this.setState({ modalBackup: false })
  }

  onBackupModalOpen = () => {
    this.setState({ modalBackup: true })
  }

  onActivity = () => {
    this.setState({ modalActivity: true })
  }

  onActivityClose = () => {
    this.setState({ modalActivity: false })
  }

  render () {
    const { content, walletState } = this.props
    const self = this

    return (
      <>
        <WalletWrapper
          balance={'25.0'}
          converted={'6.00 USD'}
          actions={[
            {
              name: 'Add funds',
              action: doNothing,
              icon: <WalletAddIcon />,
              testId: 'panel-add-funds',
              externalWallet: true
            },
            {
              name: 'Withdraw Funds',
              action: doNothing,
              icon: <WalletWithdrawIcon />,
              externalWallet: true
            }
          ]}
          compact={false}
          contentPadding={false}
          onSettingsClick={this.onBackupModalOpen}
          showCopy={true}
          showSecActions={true}
          walletState={walletState}
          walletProvider={'Uphold'}
        >
          {
            content === 'empty' ? <WalletEmpty /> : null
          }
          {
            content === 'summary'
              ? <WalletSummary
                report={{
                  grant: { tokens: '10.0', converted: '0.25' },
                  ads: { tokens: '10.0', converted: '0.25' },
                  deposit: { tokens: '10.0', converted: '0.25' },
                  contribute: { tokens: '10.0', converted: '0.25' },
                  donation: { tokens: '2.0', converted: '0.25' },
                  tips: { tokens: '19.0', converted: '5.25' }
                }}
                onActivity={this.onActivity}
              />
              : null
          }
        </WalletWrapper>
        {
          this.state.modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.activeTabId}
              backupKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
              showBackupNotice={false}
              walletProvider={'Uphold'}
              onTabChange={this.onBackupTabChange.bind(self)}
              onClose={this.onBackupModalClose.bind(self)}
              onCopy={doNothing}
              onPrint={doNothing}
              onSaveFile={doNothing}
              onRestore={doNothing}
              onReset={doNothing}
              internalFunds={0}
            />
            : null
        }
        {
          this.state.modalActivity
            ? <ModalActivity
                activityRows={this.activityContributions}
                transactionRows={this.activityTransactions}
                onClose={this.onActivityClose}
                onPrint={doNothing}
                onMonthChange={doNothing}
                months={{
                  'jun-2018': 'June 2018',
                  'may-2018': 'May 2018',
                  'apr-2018': 'April 2018'
                }}
                summary={[
                  {
                    type: 'grant',
                    token: {
                      value: '10.0',
                      converted: '5.20'
                    }
                  },
                  {
                    type: 'ads',
                    token: {
                      value: '10.0',
                      converted: '5.20'
                    }
                  },
                  {
                    type: 'contribute',
                    token: {
                      value: '10.0',
                      converted: '5.20'
                    }
                  },
                  {
                    type: 'monthly',
                    token: {
                      value: '2.0',
                      converted: '1.1'
                    }
                  },
                  {
                    type: 'tip',
                    token: {
                      value: '19.0',
                      converted: '10.10'
                    }
                  }
                ]}
            />
            : null
        }
      </>
    )
  }
}

export default PageWallet
