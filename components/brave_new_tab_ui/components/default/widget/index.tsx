/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
  paddingType: 'none' | 'right' | 'default'
  onLearnMore?: () => void
  onAddSite?: () => void
  customLinksEnabled?: boolean
  onToggleCustomLinksEnabled?: () => void
}

export interface WidgetState {
  widgetMenuPersist: boolean
}

export function Widget ({
  menuPosition,
  hideWidget,
  textDirection,
  preventFocus,
  isCrypto,
  isCryptoTab,
  widgetTitle,
  hideMenu,
  isForeground,
  paddingType,
  onLearnMore,
  onAddSite,
  customLinksEnabled,
  onToggleCustomLinksEnabled,
  children
}: WidgetProps & { children: React.ReactNode }) {
  const [widgetMenuPersist, setWidgetMenuPersist] = React.useState(!!isForeground)
  return <StyledWidgetContainer menuPosition={menuPosition} textDirection={textDirection}>
    <StyledWidget
      isCrypto={isCrypto}
      isCryptoTab={isCryptoTab}
      widgetMenuPersist={widgetMenuPersist}
      preventFocus={preventFocus}
      paddingType={paddingType}>
      {children}
    </StyledWidget>
    {hideWidget && !hideMenu && !preventFocus &&
      <WidgetMenu
        widgetTitle={widgetTitle}
        onLearnMore={onLearnMore}
        onAddSite={onAddSite}
        customLinksEnabled={customLinksEnabled}
        onToggleCustomLinksEnabled={onToggleCustomLinksEnabled}
        isForeground={isForeground}
        widgetMenuPersist={widgetMenuPersist}
        textDirection={textDirection}
        menuPosition={menuPosition}
        hideWidget={hideWidget}
        persistWidget={() => setWidgetMenuPersist(true)}
        unpersistWidget={() => setWidgetMenuPersist(false)}
        paddingType={paddingType} />}
  </StyledWidgetContainer>
}

const createWidget = <P extends object>(WrappedComponent: React.ComponentType<P>) =>
  (props: P & WidgetProps) => <Widget {...props as WidgetProps}>
    <WrappedComponent {...props as P}/>
  </Widget>

export default createWidget
