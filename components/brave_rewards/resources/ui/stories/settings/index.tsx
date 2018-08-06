/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Grid, Column } from '../../../../src/components/layout/gridList'
import Box from '../../../../src/features/rewards/box/index'
import Alert from '../../../../src/features/rewards/alert'
import TableDonation, { DetailRow as DonationDetailRow } from '../../../../src/features/rewards/tableDonation'
import ModalContribute from '../../../../src/features/rewards/modalContribute'
import ModalBackupRestore, { TabsType } from '../../../../src/features/rewards/modalBackupRestore'
import WalletEmpty from '../../../../src/features/rewards/walletEmpty'
import WalletSummary from '../../../../src/features/rewards/walletSummary'
import WalletOff from '../../../../src/features/rewards/walletOff'
import SettingsPage from '../../../../src/features/rewards/settingsPage'
import GrantClaim from '../../../../src/features/rewards/grantClaim'
import GrantInit from '../../../../src/features/rewards/grantInit'
import GrantWrapper from '../../../../src/features/rewards/grantWrapper'
import GrantCaptcha from '../../../../src/features/rewards/grantCaptcha'
import GrantComplete from '../../../../src/features/rewards/grantComplete'
import List from '../../../../src/features/rewards/list'
import Tokens from '../../../../src/features/rewards/tokens'
import Select from '../../../../src/components/formControls/select'
import Checkbox from '../../../../src/components/formControls/checkbox'
import DisabledContent from '../../../../src/features/rewards/disabledContent'
import MainToggle from '../../../../src/features/rewards/mainToggle'
import WalletWrapper from '../../../../src/features/rewards/walletWrapper'
import TableContribute, { DetailRow as ContributeDetailRow } from '../../../../src/features/rewards/tableContribute'
import { DetailRow as TransactionsRow } from '../../../../src/features/rewards/tableTransactions'

// Assets
import locale from './fakeLocale'
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'
import { boolean, select, object } from '@storybook/addon-knobs'
import ModalActivity from '../../../../src/features/rewards/modalActivity'

// Images
const adsImg = require('../../../assets/img/rewards_ads.svg')
const contributeImg = require('../../../assets/img/rewards_contribute.svg')
const wallet = require('../../../assets/img/rewards_wallet.svg')
const funds = require('../../../assets/img/rewards_funds.svg')
const bartBaker = require('../../../assets/img/bartBaker.jpeg')
const ddgo = require('../../../assets/img/ddgo.jpg')
const wiki = require('../../../assets/img/wiki.jpg')
const buzz = require('../../../assets/img/buzz.jpg')
const guardian = require('../../../assets/img/guardian.jpg')
const eich = require('../../../assets/img/eich.jpg')
const captchaDrop = require('../../../assets/img/captchaDrop.png')

interface State {
  adsToggle: boolean
  contributeToggle: boolean
  mainToggle: boolean
  modalContribute: boolean
  modalBackup: boolean
  modalBackupActive: TabsType
  modalActivity: boolean
  grantShow: boolean
  grantStep: string
}

const doNothing = () => {
  console.log('nothing')
}

class Settings extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      adsToggle: false,
      contributeToggle: true,
      mainToggle: true,
      modalContribute: false,
      modalBackup: false,
      modalBackupActive: 'backup',
      modalActivity: false,
      grantShow: true,
      grantStep: ''
    }
  }

  get activityContributions (): ContributeDetailRow[] {
    return [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bartBaker
        },
        attention: 40,
        onRemove: doNothing,
        token: {
          value: 5,
          converted: 5
        }
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        attention: 20,
        onRemove: doNothing,
        token: {
          value: 4,
          converted: 11
        }
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        attention: 10,
        onRemove: doNothing,
        token: {
          value: 3,
          converted: 15
        }
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        attention: 5,
        onRemove: doNothing,
        token: {
          value: 2,
          converted: 17
        }
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        attention: 4,
        onRemove: doNothing,
        token: {
          value: 1,
          converted: 11
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
          value: 5,
          converted: 5
        }
      },
      {
        date: '6/9',
        type: 'tipOnLike',
        description: {
          publisher: 'Bart Baker',
          platform: 'YouTube'
        },
        amount: {
          isNegative: true,
          value: 5,
          converted: 11
        }
      },
      {
        date: '6/10',
        type: 'deposit',
        description: 'Token grant made available or unlocked',
        amount: {
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/12',
        type: 'donation',
        description: 'coinmarketcap.com',
        amount: {
          isNegative: true,
          value: 10,
          converted: 15
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
          value: 1,
          converted: 2
        }
      },
      {
        date: '6/26',
        type: 'deposit',
        description: 'Added via Uphold',
        amount: {
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/31',
        type: 'contribute',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: 10,
          converted: 15
        }
      },
      {
        date: '6/31',
        type: 'recurringDonation',
        description: 'Monthly payment',
        amount: {
          isNegative: true,
          value: 5,
          converted: 15
        }
      }
    ]
  }

  get contributeRows (): ContributeDetailRow[] {
    return [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bartBaker
        },
        attention: 40,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        attention: 20,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        attention: 10,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        attention: 5,
        onRemove: doNothing
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        attention: 4,
        onRemove: doNothing
      }
    ]
  }

  get donationRows (): DonationDetailRow[] {
    return [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bartBaker
        },
        type: 'recurring',
        contribute: {
          tokens: 2,
          converted: 0.2
        },
        onRemove: doNothing
      },
      {
        profile: {
          verified: false,
          name: 'theguardian.com',
          src: guardian
        },
        type: 'donation',
        contribute: {
          tokens: 12,
          converted: 6.2
        },
        text: 'May 7'
      },
      {
        profile: {
          verified: false,
          name: 'BrendanEich',
          provider: 'twitter',
          src: eich
        },
        type: 'tip',
        contribute: {
          tokens: 7,
          converted: 3.2
        },
        text: 'May 2'
      }
    ]
  }

  adsDisabled () {
    return (
      <DisabledContent
        image={adsImg}
        theme={{ color: '#ceb4e1', boldColor: '#b490cf' }}
      >
        <h3>Coming soon.</h3>
      </DisabledContent>
    )
  }

  contributeSettingsChild = () => {
    return (
      <>
        <Grid columns={1} theme={{ maxWidth: '270px', margin: '0 auto' }}>
            <Column size={1} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
              <Select title={locale.contributionMonthly}>
                <div data-value='10'><Tokens value={10} converted={4}/></div>
                <div data-value='20'><Tokens value={20} converted={6}/></div>
                <div data-value='40'><Tokens value={40} converted={12}/></div>
                <div data-value='100'><Tokens value={100} converted={40}/></div>
              </Select>
               <Select title={locale.contributionSitesLimit}>
                <div data-value='0'>{locale.contributionSitesNoLimit}</div>
                <div data-value='10'>{locale.contributionSitesLimit10}</div>
                <div data-value='50'>{locale.contributionSitesLimit50}</div>
              </Select>
            </Column>
          </Grid>
      </>
    )
  }

  contributeDisabled () {
    return (
      <DisabledContent
        image={contributeImg}
        theme={{ color: '#ce9ccf', boldColor: '#c16fc2' }}
      >
        • Pay directly for the content you love. <br/>
        • Your <b>monthly allowance</b> gets divided based on your attention metric.
      </DisabledContent>
    )
  }

  donationSettingsChild = () => {
    return (
      <>
        <Grid columns={1} theme={{ maxWidth: '270px', margin: '0 auto' }}>
            <Column size={1} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
              <Checkbox
                title={'Enable ability to give tips on ‘Like’ posts'}
                value={{ 'yt': true, 'tw': false, 'inst': false }}
                multiple={true}
              >
                <div data-key='yt'>YouTube</div>
                <div data-key='tw'>Twitter</div>
                <div data-key='inst'>Instagram</div>
              </Checkbox>
            </Column>
          </Grid>
      </>
    )
  }

  onMainToggle = () => {
    this.setState({ mainToggle: !this.state.mainToggle })
  }

  onContributeToggle = () => {
    this.setState({ contributeToggle: !this.state.contributeToggle })
  }

  onContributeModalClose = () => {
    this.setState({ modalContribute: false })
  }

  onContributeModalOpen = () => {
    this.setState({ modalContribute: true })
  }

  onBackupTabChange = (tabId: TabsType) => {
    this.setState({ modalBackupActive: tabId })
  }

  onBackupModalClose = () => {
    this.setState({ modalBackup: false })
  }

  onBackupModalOpen = () => {
    this.setState({ modalBackup: true })
  }

  onGrantShow = () => {
    this.setState({ grantShow: false, grantStep: 'init' })
  }

  onGrantHide = () => {
    this.setState({ grantStep: '' })
  }

  onGrantStep = (step: string) => {
    this.setState({ grantStep: step })
  }

  onActivity = () => {
    this.setState({ modalActivity: true })
  }

  onActivityClose = () => {
    this.setState({ modalActivity: false })
  }

  render () {
    const showNotification = boolean('Show notification', false)
    const content = select(
      'Content',
      {
        empty: 'empty',
        summary: 'summary',
        off: 'off'
      },
      'empty' as 'empty' | 'summary' | 'off'
    )
    const self = this

    return (
      <SettingsPage>
        <Grid columns={3} theme={{ gridGap: '32px' }}>
          <Column size={2} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <MainToggle
              onToggle={this.onMainToggle}
              enabled={this.state.mainToggle}
            />
            <Box
              title={locale.adsTitle}
              theme={{ titleColor: '#C12D7C' }}
              description={locale.adsDesc}
              toggle={false}
              disabledContent={this.adsDisabled()}
            />
            <Box
              title={locale.contributionTitle}
              theme={{ titleColor: '#9F22A1' }}
              description={locale.contributionDesc}
              toggle={true}
              checked={this.state.contributeToggle}
              settingsChild={this.contributeSettingsChild()}
              disabledContent={this.contributeDisabled()}
              onToggle={this.onContributeToggle}
            >
              {
                this.state.modalContribute
                ? <ModalContribute
                  rows={this.contributeRows}
                  onClose={this.onContributeModalClose.bind(self)}
                />
                : null
              }
              <List title={locale.contributionMonthly}>
                <Select
                  theme={{
                    border: 'none',
                    padding: '0',
                    arrowPadding: '0',
                    maxWidth: '100%'
                  }}
                >
                  <div data-value='10'><Tokens value={10} converted={4}/></div>
                  <div data-value='20'><Tokens value={20} converted={6}/></div>
                  <div data-value='40'><Tokens value={40} converted={12}/></div>
                  <div data-value='100'><Tokens value={100} converted={40}/></div>
                </Select>
              </List>
              <List
                title={locale.contributionNextDate}
                theme={{
                  'font-size': '14px',
                  'text-align': 'right',
                  'border-radius': '6px',
                  color: '#4b4c5c',
                  background: '#e9f0ff',
                  display: 'inline-block',
                  padding: '9px 10px 9px 13px'
                }}
              >
                July 25th
              </List>
              <List title={locale.contributionSites}>
                Total &nbsp;<Tokens value={55} hideText={true} toFixed={false}/>
              </List>
              <TableContribute
                header={[
                  'Site visited',
                  'Attention score',
                  ''
                ]}
                rows={this.contributeRows}
                allSites={false}
                numSites={55}
                onShowAll={this.onContributeModalOpen.bind(self)}
                theme={{
                  headerColor: '#9F22A1'
                }}
              >
                Please visit some sites
              </TableContribute>
            </Box>
            <Box
              title={locale.donationTitle}
              theme={{ titleColor: '#696FDC' }}
              description={locale.donationDesc}
              settingsChild={this.donationSettingsChild()}
            >
              <List title={locale.donationTotal}>
                <Tokens value={21} converted={7} />
              </List>
              <List title={locale.donationList}>
                Total &nbsp;<Tokens value={3} hideText={true} toFixed={false} />
              </List>
              <TableDonation
                rows={this.donationRows}
                allItems={true}
                theme={{
                  headerColor: '#696FDC'
                }}
              >
                Please visit some sites
              </TableDonation>
            </Box>
          </Column>
          <Column size={1} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            {
              this.state.modalBackup
              ? <ModalBackupRestore
                activeTabId={this.state.modalBackupActive}
                recoveryKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
                onTabChange={this.onBackupTabChange.bind(self)}
                onClose={this.onBackupModalClose.bind(self)}
                onCopy={doNothing}
                onPrint={doNothing}
                onSaveFile={doNothing}
                onRestore={doNothing}
                onImport={doNothing}
              />
              : null
            }
            {
              this.state.grantShow
              ? <GrantClaim onClick={this.onGrantShow}/>
              : null
            }
            {
              this.state.grantStep === 'init'
              ? <GrantWrapper
                  onClose={this.onGrantHide}
                  title={'Good news!'}
                  text={'Free 30 BAT have been awarded to you so you can support more publishers.'}
              >
                <GrantInit onAccept={this.onGrantStep.bind(this, 'captcha')} onLater={this.onGrantHide} />
              </GrantWrapper>
              : null
            }
            {
              this.state.grantStep === 'captcha'
              ? <GrantWrapper
                  onClose={this.onGrantHide}
                  title={'Almost there…'}
                  text={'Prove that you are human!'}
              >
                <GrantCaptcha onSolution={this.onGrantStep.bind(this, 'complete')} dropBgImage={captchaDrop} />
              </GrantWrapper>
              : null
            }
            {
              this.state.grantStep === 'complete'
              ? <GrantWrapper
                onClose={this.onGrantHide}
                title={'It’s your lucky day!'}
                text={'Your token grant is on its way.'}
              >
                <GrantComplete onClose={this.onGrantHide} amount={30} date={'8/15/2018'} />
              </GrantWrapper>
              : null
            }
            <WalletWrapper
              tokens={25}
              converted={'6.0 USD'}
              actions={[
                {
                  name: 'Add funds',
                  action: doNothing,
                  icon: wallet
                },
                {
                  name: 'Withdraw Funds',
                  action: doNothing,
                  icon: funds
                }
              ]}
              onSettingsClick={this.onBackupModalOpen}
              onActivityClick={doNothing}
              showCopy={true}
              showSecActions={true}
              grants={object('Claimed grants', [
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
              ])}
              connectedWallet={boolean('Connected wallet', false)}
            >
              {
                showNotification
                ? <Alert type={'error'} theme={{ position: 'absolute' }}>
                    <b>Funds received!</b> 25 tokens are added to your wallet successfully.
                </Alert>
                : null
              }
              {
                content === 'empty' ? <WalletEmpty/> : null
              }
              {
                content === 'summary'
                ? <WalletSummary
                  grant={{ color: '#C12D7C', tokens: 10, converted: 0.25 }}
                  ads={{ color: '#C12D7C', tokens: 10, converted: 0.25 }}
                  contribute={{ color: '#9752CB', tokens: 10, converted: 0.25 }}
                  donation={{ color: '#4C54D2', tokens: 2, converted: 0.25 }}
                  tips={{ color: '#4C54D2', tokens: 19, converted: 5.25 }}
                  onActivity={this.onActivity}
                />
                : null
              }
              {
                content === 'off' ? <WalletOff/> : null
              }
            </WalletWrapper>
            {
              this.state.modalActivity
              ? <ModalActivity
                contributeRows={this.activityContributions}
                transactionRows={this.activityTransactions}
                onClose={this.onActivityClose}
                onPrint={doNothing}
                onDownloadPDF={doNothing}
                onMonthChange={doNothing}
                months={{ 'jun-2018': 'June 2018', 'may-2018': 'May 2018', 'apr-2018': 'April 2018' }}
                currentMonth={'jun-2018'}
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
          </Column>
        </Grid>
      </SettingsPage>
    )
  }
}

export default Settings
