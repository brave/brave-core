/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledContent,
  StyledGrid,
  StyledOneColumn,
  StyledTwoColumn
} from './style'

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
        <StyledGrid>
          <StyledOneColumn>
            <img src={image} />
          </StyledOneColumn>
          <StyledTwoColumn>
            <StyledContent type={type}>
              {children}
            </StyledContent>
          </StyledTwoColumn>
        </StyledGrid>
      </div>
    )
  }
}
