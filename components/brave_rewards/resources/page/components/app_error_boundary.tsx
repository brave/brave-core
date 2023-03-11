/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppError } from './app_error'

interface Props {
  children: React.ReactNode
}

interface State {
  error: Error | null
}

export class AppErrorBoundary extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { error: null }
  }

  static getDerivedStateFromError (error: unknown) {
    return { error: error instanceof Error ? error : new Error(String(error)) }
  }

  render () {
    return this.state.error
      ? <AppError error={this.state.error} />
      : this.props.children
  }
}
