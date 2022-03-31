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
  hideMenu?: boolean
  isForeground?: boolean
  lightWidget?: boolean
  paddingType: 'none' | 'right' | 'default'
  onLearnMore?: () => void
  onDisconnect?: () => void
  onRefreshData?: () => void
  onAddSite?: () => void
  customLinksEnabled?: boolean
  onToggleCustomLinksEnabled?: () => void
}

export interface WidgetState {
  widgetMenuPersist: boolean
}

const createWidget = <P extends object>(WrappedComponent: React.ComponentType<P>) =>
  class Widget extends React.Component<P & WidgetProps, WidgetState> {
    constructor (props: P & WidgetProps) {
      super(props)
      this.state = {
        widgetMenuPersist: !!props.isForeground
      }
    }

    persistWidget = () => {
      this.setState({ widgetMenuPersist: true })
    }

    unpersistWidget = () => {
      this.setState({ widgetMenuPersist: false })
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
        hideMenu,
        isForeground,
        lightWidget,
        paddingType,
        onLearnMore,
        onDisconnect,
        onRefreshData,
        onAddSite,
        customLinksEnabled,
        onToggleCustomLinksEnabled
      } = this.props
      const { widgetMenuPersist } = this.state

      return (
        <StyledWidgetContainer
          menuPosition={menuPosition}
          textDirection={textDirection}
        >
          <StyledWidget
            isCrypto={isCrypto}
            isCryptoTab={isCryptoTab}
            widgetMenuPersist={widgetMenuPersist}
            preventFocus={preventFocus}
            paddingType={paddingType}
          >
            <WrappedComponent {...this.props as P}/>
          </StyledWidget>
          {hideWidget && !hideMenu && !preventFocus &&
          <WidgetMenu
            widgetTitle={widgetTitle}
            onLearnMore={onLearnMore}
            onDisconnect={onDisconnect}
            onRefreshData={onRefreshData}
            onAddSite={onAddSite}
            customLinksEnabled={customLinksEnabled}
            onToggleCustomLinksEnabled={onToggleCustomLinksEnabled}
            isForeground={isForeground}
            widgetMenuPersist={widgetMenuPersist}
            textDirection={textDirection}
            menuPosition={menuPosition}
            hideWidget={hideWidget}
            persistWidget={this.persistWidget}
            unpersistWidget={this.unpersistWidget}
            lightWidget={lightWidget}
            paddingType={paddingType}
          />
          }
        </StyledWidgetContainer>
      )
    }
  }

export default createWidget
