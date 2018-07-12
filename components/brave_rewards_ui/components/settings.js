/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  Box,
  Column,
  ContributeTable, DisabledContent,
  DonationTable,
  Grid,
  List,
  MainToggle, ModalBackupRestore,
  ModalContribute, Panel, PanelEmpty,
  Select, SettingsPage,
  Tokens
} from 'brave-ui'
import { getLocale } from '../../common/locale'
const React = require('react')
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const rewardsActions = require('../actions/rewards_actions')

const walletIcon = require('../../img/rewards/wallet_icon.svg')
const fundsIcon = require('../../img/rewards/funds_icon.svg')
const adsDisabledIcon = require('../../img/rewards/ads_disabled.svg')
const contributeDisabledIcon = require('../../img/rewards/contribute_disabled.svg')

// TODO temp, remove
const bartBaker = require('../../img/rewards/temp/bartBaker.jpeg')

class RewardsPage extends React.Component {
  constructor (props) {
    super(props)
    this.state = {
      adsToggle: false,
      contributeToggle: true,
      mainToggle: true,
      modalContribute: false,
      modalBackup: false,
      modalBackupActive: 'backup'
    }
  }

  // TODO remove
  get contributeRows () {
    return [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bartBaker
        },
        attention: 40,
        onRemove: () => {}
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: bartBaker
        },
        attention: 20,
        onRemove: () => {
        }
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: bartBaker
        },
        attention: 10,
        onRemove: () => {}
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: bartBaker
        },
        attention: 5,
        onRemove: () => {}
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: bartBaker
        },
        attention: 4,
        onRemove: () => {}
      }
    ]
  }

  // TODO remove
  get donationRows () {
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
        onRemove: () => {}
      },
      {
        profile: {
          verified: false,
          name: 'theguardian.com',
          src: bartBaker
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
          src: bartBaker
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
    return <DisabledContent
      image={adsDisabledIcon}
      theme={{color: '#ceb4e1', boldColor: '#b490cf'}}
    >
      <h3>{getLocale('adsDisabledText')}</h3>
    </DisabledContent>
  }

  contributeDisabled () {
    return <DisabledContent
      image={contributeDisabledIcon}
      theme={{color: '#ce9ccf', boldColor: '#c16fc2'}}
    >
      • {getLocale('contributionDisabledText1')} <br />
      • {getLocale('contributionDisabledText2')}}
    </DisabledContent>
  }

  render () {
    // const { rewardsData } = this.props
    const self = this

    return <SettingsPage>
      <Grid columns={3} theme={{gridGap: '32px'}}>
        <Column size={2} theme={{justifyContent: 'center', flexWrap: 'wrap'}}>
          <MainToggle
            onToggle={() => { this.setState({mainToggle: !this.state.mainToggle}) }}
            enabled={this.state.mainToggle}
          />
          <Box
            title={getLocale('adsTitle')}
            theme={{titleColor: '#C12D7C'}}
            description={getLocale('adsDesc')}
            toggle={false}
            disabledContent={this.adsDisabled()}
          />
          <Box
            title={getLocale('contributionTitle')}
            theme={{titleColor: '#9F22A1'}}
            description={getLocale('contributionDesc')}
            toggle
            checked={this.state.contributeToggle}
            disabledContent={this.contributeDisabled()}
            onToggle={() => { this.setState({contributeToggle: !this.state.contributeToggle}) }}
            settingsChild={'TODO Add me'}
          >
            {
              this.state.modalContribute
              ? <ModalContribute
                rows={this.contributeRows}
                onClose={() => self.setState({modalContribute: false})}
              />
              : null
            }
            <List title={getLocale('contributionMonthly')}>
              <Select
                onChange={() => false}
                theme={{
                  border: 'none',
                  padding: '0 20px 0 0',
                  arrowPadding: '0'
                }}
              >
                <div data-value='10'><Tokens value={10} converted={'4'} /></div>
                <div data-value='20'><Tokens value={20} converted={'6'} /></div>
                <div data-value='40'><Tokens value={40} converted={'12'} /></div>
                <div data-value='100'><Tokens value={100} converted={'40'} /></div>
              </Select>
            </List>
            <List title={getLocale('contributionNextDate')} theme={{
              'font-size': '16px',
              'font-weight': '600',
              'text-align': 'right',
              'border-radius': '6px',
              color: '#4b4c5c',
              background: '#e9f0ff',
              display: 'inline-block',
              padding: '9px 10px 9px 13px'
            }}>
              July 25th
            </List>
            <List title={getLocale('contributionSites')}>
              Total &nbsp;<Tokens value={55} hideText />
            </List>
            <ContributeTable
              header={[
                getLocale('contributionSiteVisited'),
                getLocale('contributionSiteAttention')
              ]}
              rows={this.contributeRows}
              allSites={false}
              numSites={55}
              onShowAll={() => self.setState({modalContribute: true})}
              theme={{
                headerColor: '#9F22A1'
              }}
            >
              {getLocale('contributionVisitSome')}
            </ContributeTable>
          </Box>
          <Box
            title={getLocale('donationTitle')}
            theme={{titleColor: '#696FDC'}}
            description={getLocale('donationDesc')}
            settingsChild={'TODO Add me'}
          >
            <List title={getLocale('donationTotalDonations')}>
              <Tokens value={21} converted={7} />
            </List>
            <List title={getLocale('donationList')}>
              {getLocale('donationTotal')} &nbsp;<Tokens value={3} hideText />
            </List>
            <DonationTable
              rows={this.donationRows}
              allItems
              theme={{
                headerColor: '#696FDC'
              }}
            >
              {getLocale('donationVisitSome')}
            </DonationTable>
          </Box>
        </Column>
        <Column size={1}>
          {
              this.state.modalBackup
              ? <ModalBackupRestore
                activeTabId={this.state.modalBackupActive}
                recoveryKey={'crouch  hint  glow  recall  round  angry  weasel  luggage save  hood  census  near  still   power  vague  balcony camp  law  now  certain  wagon  affair  butter  choice '}
                onTabChange={(tabId) => self.setState({ modalBackupActive: tabId })}
                onClose={() => self.setState({modalBackup: false})}
                onCopy={() => {}}
                onPrint={() => {}}
                onSaveFile={() => {}}
                onRestore={() => {}}
                onImport={() => {}}
                />
              : null
            }
          <Panel
            tokens={25}
            converted={'6.0 USD'}
            actions={[
              {
                name: getLocale('panelAddFunds'),
                action: () => {},
                icon: walletIcon
              },
              {
                name: getLocale('panelWithdrawFunds'),
                action: () => {},
                icon: fundsIcon
              }
            ]}
            onSettingsClick={() => self.setState({modalBackup: true})}
            onActivityClick={() => {}}
            showCopy
            showSecActions
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
        </Column>
      </Grid>
    </SettingsPage>
  }
}

const mapStateToProps = (state) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsPage)
