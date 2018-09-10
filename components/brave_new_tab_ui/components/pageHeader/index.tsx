/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledPageHeader } from './style'

export interface PageHeaderProps {
  testId?: string
  children: React.ReactNode
}

/**
 * Styled block around the stats and clock component
 * @prop {string} testId - the component's id used for testing purposes
 * @prop {React.ReactNode} children - the child elements
 */
export default class PageHeader extends React.PureComponent<PageHeaderProps, {}> {
  render () {
    const { children } = this.props
    return (
      <StyledPageHeader>
        {children}
      </StyledPageHeader>
    )
  }
}
