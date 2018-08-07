/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import {
  Box,
  DisabledContent,
  List,
  ModalContribute,
  TableContribute,
  Tokens
} from '../../../../src/features/rewards'
import { Column, Grid, Select } from '../../../../src/components'
import { DetailRow as ContributeDetailRow } from '../../../../src/features/rewards/tableContribute'

// Utils
import locale from './fakeLocale'

// Assets
const bartBaker = require('../../../assets/img/bartBaker.jpeg')
const buzz = require('../../../assets/img/buzz.jpg')
const contributeImg = require('../../../assets/img/rewards_contribute.svg')
const ddgo = require('../../../assets/img/ddgo.jpg')
const guardian = require('../../../assets/img/guardian.jpg')
const wiki = require('../../../assets/img/wiki.jpg')

const doNothing = () => {
  console.log('nothing')
}

interface State {
  contributeToggle: boolean
  modalContribute: boolean
}

class ContributeBox extends React.Component<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      contributeToggle: true,
      modalContribute: false
    }
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

  onContributeToggle = () => {
    this.setState({ contributeToggle: !this.state.contributeToggle })
  }

  onContributeModalClose = () => {
    this.setState({ modalContribute: false })
  }

  onContributeModalOpen = () => {
    this.setState({ modalContribute: true })
  }

  render () {
    return (
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
    )
  }
}

export default ContributeBox
