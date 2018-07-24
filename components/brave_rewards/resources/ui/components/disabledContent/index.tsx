/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as CSS from 'csstype'
import { StyledWrapper, StyledImage, StyledContent } from './style'
import { Column, Grid } from '../../../components/layout/gridList/index'

interface Theme {
  color?: CSS.Color
  boldColor?: CSS.Color
}

export interface Props {
  children: React.ReactNode
  id?: string
  image?: string
  theme?: Theme
}

export default class DisabledContent extends React.PureComponent<Props, {}> {
  render () {
    const { id, image, theme, children } = this.props

    return (
      <StyledWrapper id={id}>
        <Grid columns={3} theme={{ gridGap: '32px', alignItems: 'center' }}>
          <Column size={1} theme={{ justifyContent: 'flex-end' }}>
            <StyledImage src={image} />
          </Column>
          <Column size={2}>
            <StyledContent theme={theme}>
              {children}
            </StyledContent>
          </Column>
        </Grid>
      </StyledWrapper>
    )
  }
}
