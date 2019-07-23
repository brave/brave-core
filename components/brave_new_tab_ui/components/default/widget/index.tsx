/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWidgetContainer } from './styles'

export interface WidgetProps {
  showWidget: boolean
}

const createWidget = <P extends object>(WrappedComponent: React.ComponentType<P>) =>
  class Widget extends React.Component<P & WidgetProps> {
    render () {
      const { showWidget } = this.props
      return (
        <StyledWidgetContainer
          showWidget={showWidget}
        >
          <WrappedComponent {...this.props as P}/>
        </StyledWidgetContainer>
      )
    }
  }

export default createWidget
