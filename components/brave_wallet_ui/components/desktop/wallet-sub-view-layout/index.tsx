// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { StyledWrapper } from './style'

export interface Props {
  noPadding?: boolean
  children?: React.ReactNode
}

export default class WalletSubViewLayout extends React.PureComponent<Props, {}> {
  render () {
    const { children, noPadding } = this.props
    return (
      <StyledWrapper noPadding={noPadding}>
        {children}
      </StyledWrapper>
    )
  }
}
