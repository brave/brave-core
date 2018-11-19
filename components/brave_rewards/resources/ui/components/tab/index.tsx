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

export interface Props {
  tabTitles?: string[]
  testId?: string
  tabIndexSelected?: number
  onChange?: (event: React.MouseEvent<HTMLDivElement>) => void
}

export default class Tab extends React.PureComponent<Props, {}> {
  static defaultProps = {
    tabIndexSelected: 0
  }

  getTabs = (tabTitles: string[]) => {
    const tabs: React.ReactNode[] = tabTitles.map((title: string, i: number) => {
      return (
        <StyledTab
          left={i === 0}
          key={`tab-${i}`}
          onClick={this.onSwitchChange.bind(this, i)}
        >
          <StyledText selected={i === this.props.tabIndexSelected}>
            {title}
          </StyledText>
        </StyledTab>
      )
    })
    return tabs
  }

  onSwitchChange = (index: number, event: React.MouseEvent<HTMLDivElement>) => {
    if (index === this.props.tabIndexSelected) {
      return
    }

    if (this.props.onChange) {
      this.props.onChange(event)
    }
  }

  render () {
    const {
      testId,
      tabTitles,
      tabIndexSelected
    } = this.props

    if (!tabTitles || tabTitles.length !== 2) {
      console.warn('Rewards Tab currently supports 2 tab titles')
      return null
    }

    return (
      <RewardsTabWrapper>
        <StyledSwitch>
          <StyledSlider data-test-id={testId}>
            {this.getTabs(tabTitles)}
          </StyledSlider>
          <StyledBullet tabIndexSelected={tabIndexSelected} />
        </StyledSwitch>
      </RewardsTabWrapper>
    )
  }
}
