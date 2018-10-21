/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  StyledStatsItemContainer,
  StyledStatsItem,
  StyledStatsItemCounter,
  StyledStatsItemText,
  StyledStatsItemDescription
} from './style'

export interface StatsProps {
  testId?: string
  children?: React.ReactNode
}

/**
 * Styled container block around stat items used in new tab page
 * @prop {string} testId - the component's id used for testing purposes
 * @prop {React.ReactNode} children - the child elements
 */
export class StatsContainer extends React.PureComponent<StatsProps, {}> {
  render () {
    const { testId, children } = this.props
    return (
      <StyledStatsItemContainer data-test-id={testId}>{children}</StyledStatsItemContainer>
    )
  }
}

export interface StatsItemProps {
  testId?: string
  counter: string | number
  text?: string
  description: string
}

/**
 * Individual stat block used in new tab page
 * @prop {string} testId - the component's id used for testing purposes
 * @prop {string | number} counter - the stat counter
 * @prop {string} text - descriptive text that goes along the stat
 * @prop {string} description - describes what the counter is showing
 */
export class StatsItem extends React.PureComponent<StatsItemProps, {}> {
  render () {
    const { testId, counter, text, description } = this.props

    return (
      <StyledStatsItem data-test-id={testId}>
        <StyledStatsItemCounter>{counter}</StyledStatsItemCounter>
          {text && <StyledStatsItemText>{text}</StyledStatsItemText>}
        <StyledStatsItemDescription>{description}</StyledStatsItemDescription>
      </StyledStatsItem>
    )
  }
}
