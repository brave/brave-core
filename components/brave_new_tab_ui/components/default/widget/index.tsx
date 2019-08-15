/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWidget, StyledWidgetContainer } from './styles'
import WidgetMenu from './widgetMenu'

export interface WidgetProps {
  showWidget: boolean
  menuPosition: 'right' | 'left'
  hideWidget: () => void
  textDirection: string
}

export interface WidgetState {
  widgetMenuPersist: boolean
}

const createWidget = <P extends object>(WrappedComponent: React.ComponentType<P>) =>
  class Widget extends React.Component<P & WidgetProps, WidgetState> {
    constructor (props: P & WidgetProps) {
      super(props)
      this.state = {
        widgetMenuPersist: false
      }
    }

    toggleWidgetHover = () => {
      this.setState({ widgetMenuPersist: !this.state.widgetMenuPersist })
    }

    unpersistWidgetHover = () => {
      this.setState({ widgetMenuPersist: false })
    }

    render () {
      const { showWidget, menuPosition, hideWidget, textDirection } = this.props
      const { widgetMenuPersist } = this.state
      return (
        <StyledWidgetContainer
          showWidget={showWidget}
          menuPosition={menuPosition}
          textDirection={textDirection}
        >
          <StyledWidget widgetMenuPersist={widgetMenuPersist}>
            <WrappedComponent {...this.props as P}/>
          </StyledWidget>
          <WidgetMenu
            widgetMenuPersist={widgetMenuPersist}
            toggleWidgetHover={this.toggleWidgetHover}
            textDirection={textDirection}
            menuPosition={menuPosition}
            hideWidget={hideWidget}
            unpersistWidgetHover={this.unpersistWidgetHover}
          />
        </StyledWidgetContainer>
      )
    }
  }

export default createWidget
