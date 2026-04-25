// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import { HeaderTitle, HeaderWrapper, TopRow, CloseButton } from './style'
import { PanelTypes } from '../../../constants/types'

export interface Props {
  title: string
  action: (path: PanelTypes) => void
}

export class PanelHeader extends React.PureComponent<Props> {
  navigate = (path: PanelTypes) => () => {
    this.props.action(path)
  }

  render() {
    const { title } = this.props
    return (
      <HeaderWrapper hasSearch={false}>
        <TopRow>
          <HeaderTitle>{title}</HeaderTitle>
          <CloseButton onClick={this.navigate('main')} />
        </TopRow>
      </HeaderWrapper>
    )
  }
}

export default PanelHeader
