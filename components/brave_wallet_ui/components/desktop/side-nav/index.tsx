// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { NavObjectType, NavTypes } from '../../../constants/types'
// Styled Components
import { StyledWrapper } from './style'

// Components
import { SideNavButton } from '../side-nav-button/index'

export interface Props {
  navList: NavObjectType[]
  selectedButton: NavTypes
  onSubmit: (id: NavTypes) => void
}

export class SideNav extends React.PureComponent<Props, {}> {
  onNav = (id: NavTypes) => () => {
    this.props.onSubmit(id)
  }

  render () {
    const { navList, selectedButton } = this.props
    return (
      <StyledWrapper>
        {navList.map((option) =>
          <SideNavButton
            key={option.id}
            isSelected={selectedButton === option.id}
            onSubmit={this.onNav(option.id)}
            text={option.name}
            icon={selectedButton === option.id ? option.primaryIcon : option.secondaryIcon}
          />
        )}
      </StyledWrapper>
    )
  }
}

export default SideNav
