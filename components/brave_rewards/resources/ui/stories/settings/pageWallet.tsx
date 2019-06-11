/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { DetailRow as ContributeDetailRow } from '../../../../src/features/rewards/tableContribute'
import { DetailRow as TransactionsRow } from '../../../../src/features/rewards/tableTransactions'

import {
  ModalActivity,
  ModalBackupRestore,
  WalletEmpty,
  WalletOff,
  WalletSummary,
  WalletWrapper
} from '../../../../src/features/rewards'
import { WalletState } from '../../../../src/features/rewards/walletWrapper'
import { WalletAddIcon, WalletWithdrawIcon } from '../../../../src/components/icons'

// Assets
const favicon = require('../../../assets/img/brave-favicon.png')
const buzz = require('../../../assets/img/buzz.jpg')
const ddgo = require('../../../assets/img/ddgo.jpg')
const guardian = require('../../../assets/img/guardian.jpg')
const wiki = require('../../../assets/img/wiki.jpg')

export interface Props {
  grants: {
    tokens: string,
    expireDate: string,
    type: string
  }[]
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

  get activityContributions (): ContributeDetailRow[] {
    return [
      {
        profile: {
          name: 'Jonathon Doe',
          verified: true,
          provider: 'youtube',
          src: favicon
        },
        url: 'https://brave.com',
        attention: 40,
        onRemove: doNothing,
        token: {
          value: '5.0',
          converted: '5.00'
        }
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        url: 'https://brave.com',
        attention: 20,
        onRemove: doNothing,
        token: {
          value: '4.0',
          converted: '11.00'
        }
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        url: 'https://brave.com',
        attention: 10,
        onRemove: doNothing,
        token: {
          value: '3.0',
          converted: '15.00'
        }
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        url: 'https://brave.com',
        attention: 5,
        onRemove: doNothing,
        token: {
          value: '2.0',
          converted: '17.00'
        }
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        url: 'https://brave.com',
        attention: 4,
        onRemove: doNothing,
        token: {
          value: '1.0',
          converted: '11.00'
        }
      }
    ]
  }

  get activityTransactions (): TransactionsRow[] {
    return [
      {
        date: '6/1',
        type: 'deposit',
        description: 'Brave Ads payment for May',
        amount: {
          value: '5.0',
          converted: '5.00'
        }
      },
      {
        date: '6/9',
        type: 'tipOnLike',
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
        date: '6/10',
        type: 'deposit',
        description: 'Token grant made available or unlocked',
        amount: {
          value: '10.0',
          converted: '15.00'
        }
      },
      {
        date: '6/12',
        type: 'donation',
        description: 'coinmarketcap.com',
        amount: {
          isNegative: true,
          value: '10.0',
          converted: '15.00'
        }
      },
      {
        date: '6/14',
        type: 'tipOnLike',
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
        date: '6/26',
        type: 'deposit',
        description: 'Added via Uphold',
        amount: {
          value: '10.0',
          converted: '15.00'
        }
      },
      {
        date: '6/31',
        type: 'contribute',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: '10.0',
          converted: '15.00'
        }
      },
      {
        date: '6/31',
        type: 'recurringDonation',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: '5.0',
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
    const { content, walletState, grants } = this.props
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
              testId: 'panel-add-funds'
            },
            {
              name: 'Withdraw Funds',
              action: doNothing,
              icon: <WalletWithdrawIcon />
            }
          ]}
          compact={false}
          contentPadding={false}
          onSettingsClick={this.onBackupModalOpen}
          onActivityClick={doNothing}
          showCopy={true}
          showSecActions={true}
          grants={grants}
          walletState={walletState}
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
          {
            content === 'off' ? <WalletOff /> : null
          }
        </WalletWrapper>
        {
          this.state.modalBackup
            ? <ModalBackupRestore
              activeTabId={this.state.activeTabId}
              backupKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
              onTabChange={this.onBackupTabChange.bind(self)}
              onClose={this.onBackupModalClose.bind(self)}
              onCopy={doNothing}
              onPrint={doNothing}
              onSaveFile={doNothing}
              onRestore={doNothing}
            />
            : null
        }
        {
          this.state.modalActivity
            ? <ModalActivity
              contributeRows={this.activityContributions}
              transactionRows={this.activityTransactions}
              onClose={this.onActivityClose}
              onPrint={doNothing}
              onDownloadPDF={doNothing}
              onMonthChange={doNothing}
              months={{
                'jun-2018': 'June 2018',
                'may-2018': 'May 2018',
                'apr-2018': 'April 2018'
              }}
              currentMonth={'jun-2018'}
              summary={[
                {
                  text: 'Token Grant available',
                  type: 'grant',
                  token: {
                    value: '10.0',
                    converted: '5.20'
                  }
                },
                {
                  text: 'Earnings from Brave Ads',
                  type: 'ads',
                  token: {
                    value: '10.0',
                    converted: '5.20'
                  }
                },
                {
                  text: 'Brave Contribute',
                  type: 'contribute',
                  notPaid: true,
                  token: {
                    value: '10.0',
                    converted: '5.20',
                    isNegative: true
                  }
                },
                {
                  text: 'Recurring Donations',
                  type: 'recurring',
                  notPaid: true,
                  token: {
                    value: '2.0',
                    converted: '1.1',
                    isNegative: true
                  }
                },
                {
                  text: 'One-time Donations/Tips',
                  type: 'donations',
                  token: {
                    value: '19.0',
                    converted: '10.10',
                    isNegative: true
                  }
                }
              ]}
              total={{
                value: '1.0',
                converted: '0.5'
              }}
              paymentDay={12}
              openBalance={{
                value: '10.0',
                converted: '5.20'
              }}
              closingBalance={{
                value: '11.0',
                converted: '5.30'
              }}
            />
            : null
        }
      </>
    )
  }
}

export default PageWallet
