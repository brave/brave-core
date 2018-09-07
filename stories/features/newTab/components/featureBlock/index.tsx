/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledFeatureBlock } from './style'

export interface FeatureBlockProps {
  testId?: string
  grid?: boolean
  children: React.ReactNode
}

/**
 * Styled block around features in the new tab page
 * divided by a grid of 3 columns
 * @prop {string} testId - the component's id used for testing purposes
 * @prop {boolean} grid - whether or not the styled block should behave like a grid
 * @prop {React.ReactNode} children - the child elements
 */
export class FeatureBlock extends React.PureComponent<FeatureBlockProps, {}> {
  render () {
    const { testId, grid, children } = this.props
    return (
      <StyledFeatureBlock data-test-id={testId} grid={grid}>
        {children}
      </StyledFeatureBlock>
    )
  }
}
