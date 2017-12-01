/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { Grid, Column, ActionButton, Anchor } from 'brave-ui'

export default class BraveShieldsFooter extends Component {
  render () {
    return (
      <Grid
        id='braveShieldsFooter'
        gap='10px'
        padding='0 10px 10px'
        background='#eee'
      >
        <Column align='flex-start' size={9}>
          <Anchor
            noStyle
            href='chrome://settings'
            target='_blank'
            text='Edit default shield settings...'
          />
        </Column>
        <Column align='flex-end' size={3}>
          <ActionButton text='Reload' />
        </Column>
      </Grid>
    )
  }
}
