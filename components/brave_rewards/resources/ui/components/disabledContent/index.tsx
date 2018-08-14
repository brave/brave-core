/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledContent } from './style'
import { Column, Grid } from '../../../components/layout/gridList'

export type Type = 'ads' | 'contribute' | 'donation'

export interface Props {
  children: React.ReactNode
  id?: string
  image?: string
  type?: Type
}

export default class DisabledContent extends React.PureComponent<Props, {}> {
  render () {
    const { id, image, children, type } = this.props

    return (
      <div id={id}>
        <Grid columns={3} theme={{ gridGap: '32px', alignItems: 'center' }}>
          <Column size={1} theme={{ justifyContent: 'flex-end' }}>
            <img src={image} />
          </Column>
          <Column size={2}>
            <StyledContent type={type}>
              {children}
            </StyledContent>
          </Column>
        </Grid>
      </div>
    )
  }
}
