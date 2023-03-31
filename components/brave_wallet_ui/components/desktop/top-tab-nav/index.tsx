// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import {
  TopTabNavObjectType,
  TabNavTypes
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  Line
} from './style'

// Components
import {
  TopTabNavButton
} from '../'

export interface Props {
  tabList: TopTabNavObjectType[]
  selectedTab?: TabNavTypes
  onSelectTab: (id: TabNavTypes) => void
}

function TopTabNav (props: Props) {
  const {
    tabList,
    selectedTab,
    onSelectTab
  } = props

  const onClickSelectTab = (id: TabNavTypes) => () => {
    onSelectTab(id)
  }

  return (
    <StyledWrapper>
      {tabList.map((option) =>
        <TopTabNavButton
          key={option.id}
          isSelected={selectedTab === option.id}
          onSubmit={onClickSelectTab(option.id)}
          text={option.name}
        />
      )}
      <Line />
    </StyledWrapper>
  )
}

export default TopTabNav
