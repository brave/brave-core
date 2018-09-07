/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledPage, StyledPageWrapper } from './style'

export interface PageProps {
  testId?: string
  children: React.ReactNode
}

/**
 * First parent wrapper around a webUI page
 * @prop {string} testId - the component's id used for testing purposes
 * @prop {React.ReactNode} children - the child elements
 */
export class Page extends React.PureComponent<PageProps, {}> {
  render () {
    const { testId, children } = this.props
    return (
      <StyledPage data-test-id={testId}>{children}</StyledPage>
    )
  }
}

/**
 * Content wrapper acting as achild of the main component <Page>
 * @prop {string} testId - the component's id used for testing purposes
 * @prop {React.ReactNode} children - the child elements
 */
export class PageWrapper extends React.PureComponent<PageProps, {}> {
  render () {
    const { testId, children } = this.props
    return (
      <StyledPageWrapper data-test-id={testId}>{children}</StyledPageWrapper>
    )
  }
}
