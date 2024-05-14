// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Components
import { PanelHeader } from '../panel-header/index'
import { PanelHeaderSlim } from '../panel-header-slim/panel-header-slim'

// Styled Components
import { StyledWrapper } from './style'

// Utils
import { PanelTypes, PanelHeaderSizes } from '../../../constants/types'

export interface Props {
  title: string
  headerStyle?: PanelHeaderSizes
  navAction: (path: PanelTypes) => void
}

export class Panel extends React.PureComponent<React.PropsWithChildren<Props>> {
  render() {
    const { title, headerStyle, navAction, children } = this.props

    return (
      <StyledWrapper>
        {headerStyle === 'slim' ? (
          <PanelHeaderSlim
            action={navAction}
            title={title}
          />
        ) : (
          <PanelHeader
            action={navAction}
            title={title}
          />
        )}
        {children}
      </StyledWrapper>
    )
  }
}

export default Panel
