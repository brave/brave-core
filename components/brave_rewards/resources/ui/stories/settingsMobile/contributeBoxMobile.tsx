/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import {
  List,
  NextContribution,
  TableContribute,
  Tokens
} from '../../../../src/features/rewards'
import {
  StyledListContent,
  StyledSitesNum,
  StyledTotalContent,
  StyledSitesLink,
  StyledMobileSettingsContainer
} from './style'
import { ControlWrapper, Checkbox } from '../../../../src/components'
import { BoxMobile, SelectMobile } from '../../../../src/features/rewards/mobile'
import { DetailRow as ContributeDetailRow } from '../../../../src/features/rewards/tableContribute'

// Utils
import locale from './fakeLocale'

// Assets
const favicon = require('../../../assets/img/brave-favicon.png')
const buzz = require('../../../assets/img/buzz.jpg')
const ddgo = require('../../../assets/img/ddgo.jpg')
const guardian = require('../../../assets/img/guardian.jpg')
const wiki = require('../../../assets/img/wiki.jpg')

interface Props {
  rewardsEnabled: boolean
}

interface State {
  allSitesShown: boolean
  contributeToggle: boolean
}

class ContributeBoxMobile extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      allSitesShown: false,
      contributeToggle: true
    }
  }

  onContributeToggle = () => {
    this.setState({
      contributeToggle: !this.state.contributeToggle
    })
  }

  onSitesShownToggle = () => {
    this.setState({
      allSitesShown: !this.state.allSitesShown
    })
  }

  onSelectSettingChange = (key: string, value: string) => {
    console.log(`${key} is now ${value}`)
  }

  doNothing = () => {
    console.log('nothing')
  }

  contributeSettingsChild = () => {
    return (
      <StyledMobileSettingsContainer>
        <ControlWrapper text={locale.contributionMonthly}>
          <SelectMobile
            onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
            amountOptions={[
              {
                value: '10.0',
                dataValue: '10',
                converted: '4.00'
              },
              {
                value: '20.0',
                dataValue: '20',
                converted: '6.00'
              },
              {
                value: '40.0',
                dataValue: '40',
                converted: '12.00'
              },
              {
                value: '60.0',
                dataValue: '60',
                converted: '17.00'
              },
              {
                value: '100.0',
                dataValue: '100',
                converted: '30.00'
              }
            ]}
          />
        </ControlWrapper>
        <ControlWrapper text={locale.contributionMinTime}>
          <SelectMobile
            onChange={this.onSelectSettingChange.bind(this, 'contributionMinTime')}
            options={[
              {
                value: '5',
                text: locale.contributionTime5
              },
              {
                value: '8',
                text: locale.contributionTime8
              },
              {
                value: '60',
                text: locale.contributionTime60
              }
            ]}
          />
        </ControlWrapper>
        <ControlWrapper text={locale.contributionMinVisits}>
          <SelectMobile
            onChange={this.onSelectSettingChange.bind(this, 'contributionMinVisits')}
            options={[
              {
                value: '1',
                text: locale.contributionVisit1
              },
              {
                value: '5',
                text: locale.contributionVisit5
              },
              {
                value: '10',
                text: locale.contributionVisit10
              }
            ]}
          />
        </ControlWrapper>
        <ControlWrapper text={locale.contributionAllowed}>
          <Checkbox
            value={{
              contributionNonVerified: true,
              contributionVideos: true
            }}
            multiple={true}
          >
            <div data-key='contributionNonVerified'>{locale.contributionNonVerified}</div>
            <div data-key='contributionVideos'>{locale.contributionVideos}</div>
          </Checkbox>
        </ControlWrapper>
      </StyledMobileSettingsContainer>
    )
  }

  get contributeRows (): ContributeDetailRow[] {
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
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        url: 'https://brave.com',
        attention: 20,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        url: 'https://brave.com',
        attention: 10,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        url: 'https://brave.com',
        attention: 5,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        url: 'https://brave.com',
        attention: 4,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: ddgo
        },
        url: 'https://brave.com',
        attention: 20,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: buzz
        },
        url: 'https://brave.com',
        attention: 10,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: guardian
        },
        url: 'https://brave.com',
        attention: 5,
        onRemove: this.doNothing
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: wiki
        },
        url: 'https://brave.com',
        attention: 4,
        onRemove: this.doNothing
      }
    ]
  }

  render () {
    const prefix = this.state.allSitesShown
      ? locale.contributionHideAll
      : locale.contributionSeeAll
    const shownRows = this.state.allSitesShown
      ? this.contributeRows
      : this.contributeRows.slice(0, 5)
    const checked = this.props.rewardsEnabled && this.state.contributeToggle

    return (
      <BoxMobile
        title={locale.contributionTitle}
        type={'contribute'}
        description={locale.contributionDesc}
        toggle={true}
        checked={checked}
        toggleAction={this.onContributeToggle}
        settingsChild={this.contributeSettingsChild()}
      >
        <List title={<StyledListContent>{locale.contributionMonthly}</StyledListContent>}>
          <StyledListContent>
            <SelectMobile
              floating={true}
              onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
              amountOptions={[
                {
                  value: '10.0',
                  dataValue: '10',
                  converted: '4.00'
                },
                {
                  value: '20.0',
                  dataValue: '20',
                  converted: '6.00'
                },
                {
                  value: '40.0',
                  dataValue: '40',
                  converted: '12.00'
                },
                {
                  value: '60.0',
                  dataValue: '60',
                  converted: '17.00'
                },
                {
                  value: '100.0',
                  dataValue: '100',
                  converted: '30.00'
                }
              ]}
            />
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{locale.contributionNextDate}</StyledListContent>}>
          <StyledListContent>
            <NextContribution>July 25th</NextContribution>
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{locale.contributionSitesNum}</StyledListContent>}>
          <StyledTotalContent>
            Total &nbsp;<Tokens value={'55'} hideText={true} />
          </StyledTotalContent>
        </List>
        <StyledListContent>
          <TableContribute
            header={[
              'Site Visited/Your Attention',
              'Exclude'
            ]}
            rows={shownRows}
            allSites={true}
            numSites={5}
            headerColor={true}
            showRowAmount={true}
            showRemove={true}
            isMobile={true}
          />
        </StyledListContent>
        <StyledSitesNum>
          <StyledSitesLink onClick={this.onSitesShownToggle}>
            {prefix} {this.contributeRows.length} {locale.contributionSitesSuffix}
          </StyledSitesLink>
        </StyledSitesNum>
      </BoxMobile>
    )
  }
}

export default ContributeBoxMobile
