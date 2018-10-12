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
  StyledSupport,
  StyledTotalContent,
  StyledSupportSites,
  StyledSitesLink
} from './style'
import { Column, Grid, Select, ControlWrapper, Checkbox } from '../../../../src/components'
import { BoxMobile } from '../../../../src/features/rewards/mobile'
import { DetailRow as ContributeDetailRow } from '../../../../src/features/rewards/tableContribute'

// Utils
import locale from './fakeLocale'

// Assets
const bartBaker = require('../../../assets/img/bartBaker.jpeg')
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

  doNothing = () => {
    console.log('nothing')
  }

  contributeSettingsChild = () => {
    return (
      <>
        <Grid columns={1} customStyle={{ maxWidth: '270px', margin: '0 auto' }}>
            <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
              <ControlWrapper text={locale.contributionMonthly}>
                <Select title={locale.contributionMonthly}>
                  <div data-value='10'><Tokens value={'10.0'} converted={'4.00'}/></div>
                  <div data-value='20'><Tokens value={'20.0'} converted={'6.00'}/></div>
                  <div data-value='40'><Tokens value={'40.0'} converted={'12.00'}/></div>
                  <div data-value='100'><Tokens value={'100.0'} converted={'40.00'}/></div>
                </Select>
              </ControlWrapper>
              <ControlWrapper text={locale.contributionMinTime}>
                <Select title={locale.contributionMinTime}>
                  <div data-value='5'>{locale.contributionTime5}</div>
                  <div data-value='8'>{locale.contributionTime8}</div>
                  <div data-value='60'>{locale.contributionTime60}</div>
                </Select>
              </ControlWrapper>
              <ControlWrapper text={locale.contributionMinVisits}>
                <Select title={locale.contributionMinVisits}>
                  <div data-value='5'>{locale.contributionVisit1}</div>
                  <div data-value='8'>{locale.contributionVisit5}</div>
                  <div data-value='60'>{locale.contributionVisit10}</div>
                </Select>
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
            </Column>
          </Grid>
      </>
    )
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
            <Select
              floating={true}
              title={locale.contributionMonthly}
            >
              <div data-value='10'><Tokens value={'10.0'} converted={'4.00'}/></div>
              <div data-value='20'><Tokens value={'20.0'} converted={'6.00'}/></div>
              <div data-value='40'><Tokens value={'40.0'} converted={'12.00'}/></div>
              <div data-value='100'><Tokens value={'100.0'} converted={'40.00'}/></div>
            </Select>
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{locale.contributionNextDate}</StyledListContent>}>
          <StyledListContent>
            <NextContribution>July 25th</NextContribution>
          </StyledListContent>
        </List>
        <StyledSupport>
          <List title={<StyledSupportSites>{locale.contributionSitesNum}</StyledSupportSites>}>
            <StyledTotalContent>
              Total &nbsp;<Tokens value={'55'} hideText={true}/>
            </StyledTotalContent>
          </List>
        </StyledSupport>
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
