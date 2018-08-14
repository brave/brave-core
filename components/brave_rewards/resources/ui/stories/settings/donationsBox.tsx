/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { Box, TableDonation, Tokens, List } from '../../../../src/features/rewards'
import { DetailRow as DonationDetailRow } from '../../../../src/features/rewards/tableDonation'
import { Column, Grid, Checkbox } from '../../../../src/components'

// Utils
import locale from './fakeLocale'

// Assets
const bartBaker = require('../../../assets/img/bartBaker.jpeg')
const eich = require('../../../assets/img/eich.jpg')
const guardian = require('../../../assets/img/guardian.jpg')

const doNothing = () => {
  console.log('nothing')
}

class DonationsBox extends React.Component {
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

  render () {
    return (
      <Box
        title={locale.donationTitle}
        type={'donation'}
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
          headerColor={true}
        >
          Please visit some sites
        </TableDonation>
      </Box>
    )
  }
}

export default DonationsBox
