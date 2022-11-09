// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { StyledWrapper, StyledContent } from './style'

export interface Props {
  maintainWidth?: boolean
  children?: React.ReactNode
}

export default class WalletPageLayout extends React.PureComponent<Props, {}> {
  render () {
    const { children, maintainWidth } = this.props
    return (
      <StyledWrapper maintainWidth={maintainWidth}>
        <StyledContent maintainWidth={maintainWidth}>
          {children}
        </StyledContent>
      </StyledWrapper>
    )
  }
}
