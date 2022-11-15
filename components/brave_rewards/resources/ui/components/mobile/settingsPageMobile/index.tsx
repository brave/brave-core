/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledContent
} from './style'

export interface Props {
  id?: string
  children?: React.ReactNode
}

export default class SettingsPageMobile extends React.PureComponent<Props, {}> {
  render () {
    const { id, children } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledContent>
          {children}
        </StyledContent>
      </StyledWrapper>
    )
  }
}
