/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWidget, StyledWidgetContainer } from './styles'
import WidgetMenu from './widgetMenu'

type HideWidgetFunction = () => void

export interface WidgetProps {
  menuPosition: 'right' | 'left'
  hideWidget?: HideWidgetFunction
  preventFocus?: boolean
  textDirection: string
  isCrypto?: boolean
  isCryptoTab?: boolean
  widgetTitle?: string
  onLearnMore?: () => void
  onDisconnect?: () => void
  onRefreshData?: () => void
}

export interface WidgetState {
  hoveringOverMenu: boolean
  widgetMenuPersist: boolean
}

const createWidget = <P extends object>(WrappedComponent: React.ComponentType<P>) =>
  class Widget extends React.Component<P & WidgetProps, WidgetState> {
    constructor (props: P & WidgetProps) {
      super(props)
      this.state = {
        hoveringOverMenu: false,
        widgetMenuPersist: false
      }
    }

    toggleWidgetHover = () => {
      this.setState({ widgetMenuPersist: !this.state.widgetMenuPersist })
    }

    persistWidgetHover = () => {
      this.setState({
        hoveringOverMenu: true,
        widgetMenuPersist: true
      })
    }

    unpersistWidgetHover = () => {
      if (!this.state.hoveringOverMenu) {
        this.setState({ widgetMenuPersist: false })
      }
    }

    disableMenu = () => {
      this.setState({
        hoveringOverMenu: false,
        widgetMenuPersist: false
      })
    }

    render () {
      const {
        menuPosition,
        hideWidget,
        textDirection,
        preventFocus,
        isCrypto,
        isCryptoTab,
        widgetTitle,
        onLearnMore,
        onDisconnect,
        onRefreshData
      } = this.props
      const { widgetMenuPersist } = this.state

      return (
        <StyledWidgetContainer
          menuPosition={menuPosition}
          textDirection={textDirection}
          onMouseLeave={this.unpersistWidgetHover}
        >
          <StyledWidget
            isCrypto={isCrypto}
            isCryptoTab={isCryptoTab}
            widgetMenuPersist={widgetMenuPersist}
            preventFocus={preventFocus}
          >
              <WrappedComponent {...this.props as P}/>
          </StyledWidget>
          {hideWidget && !preventFocus &&
          <WidgetMenu
            widgetTitle={widgetTitle}
            onLearnMore={onLearnMore}
            onDisconnect={onDisconnect}
            onRefreshData={onRefreshData}
            widgetMenuPersist={widgetMenuPersist}
            toggleWidgetHover={this.toggleWidgetHover}
            textDirection={textDirection}
            menuPosition={menuPosition}
            hideWidget={hideWidget as HideWidgetFunction}
            unpersistWidgetHover={this.unpersistWidgetHover}
            onMouseEnter={this.persistWidgetHover}
            onMouseLeave={this.disableMenu}
          />
          }
        </StyledWidgetContainer>
      )
    }
  }

export default createWidget
