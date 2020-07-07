/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  RewardsTabWrapper,
  StyledSlider,
  StyledBullet,
  StyledText,
  StyledTab,
  StyledSwitch
} from './style'

export type Type = 'contribute' | 'restore'

export interface Props {
  tabTitles?: string[]
  testId?: string
  type?: Type
  tabIndexSelected?: number
  onChange?: (newTabId: number) => void
}

export default class Tab extends React.PureComponent<Props, {}> {
  static defaultProps = {
    tabIndexSelected: 0
  }

  getTabs = (tabTitles: string[]) => {
    const { type } = this.props

    const tabs: React.ReactNode[] = tabTitles.map((title: string, i: number) => {
      return (
        <StyledTab
          key={`tab-${i}`}
          data-test-id={`${this.props.testId}-${i}`}
          onClick={this.onSwitchChange.bind(this, i)}
        >
          <StyledText
            type={type}
            selected={i === this.props.tabIndexSelected}
          >
            {title}
          </StyledText>
        </StyledTab>
      )
    })
    return tabs
  }

  onSwitchChange = (index: number) => {
    if (index === this.props.tabIndexSelected) {
      return
    }

    if (this.props.onChange) {
      this.props.onChange(index)
    }
  }

  render () {
    const {
      testId,
      tabTitles,
      tabIndexSelected
    } = this.props

    if (!tabTitles) {
      return null
    }

    return (
      <RewardsTabWrapper>
        <StyledSwitch>
          <StyledSlider data-test-id={testId}>
            {this.getTabs(tabTitles)}
          </StyledSlider>
          <StyledBullet size={tabTitles.length} tabIndexSelected={tabIndexSelected} />
        </StyledSwitch>
      </RewardsTabWrapper>
    )
  }
}
