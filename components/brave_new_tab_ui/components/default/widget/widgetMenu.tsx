// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StyledWidgetMenuContainer, StyledWidgetMenu, StyledWidgetButton, StyledWidgetIcon, StyledSpan, StyledWidgetLink, StyledEllipsis } from './styles'
import { IconButton } from '../../default'
import EllipsisIcon from './assets/ellipsis'
import HideIcon from './assets/hide'
import AddSiteIcon from './assets/add-site'
import FrecencyIcon from './assets/frecency'
import FavoritesIcon from './assets/favorites'
import LearnMoreIcon from './assets/learn-more'
import DisconnectIcon from './assets/disconnect'
import RefreshIcon from './assets/refresh'
import { getLocale } from '../../../../common/locale'

interface Props {
  menuPosition: 'right' | 'left'
  hideWidget: () => void
  textDirection: string
  widgetMenuPersist: boolean
  persistWidget: () => void
  unpersistWidget: () => void
  widgetTitle?: string
  isForeground?: boolean
  onLearnMore?: () => void
  onDisconnect?: () => void
  onRefreshData?: () => void
  onAddSite?: () => void
  customLinksEnabled?: boolean
  onToggleCustomLinksEnabled?: () => void
  lightWidget?: boolean
  paddingType: 'none' | 'right' | 'default'
}

interface State {
  showMenu: boolean
}

export default class WidgetMenu extends React.PureComponent<Props, State> {
  settingsMenuRef: React.RefObject<any>
  constructor (props: Props) {
    super(props)
    this.settingsMenuRef = React.createRef()
    this.state = {
      showMenu: false
    }
  }

  handleClickOutsideMenu = (event: Event) => {
    if (this.settingsMenuRef && !this.settingsMenuRef.current.contains(event.target)) {
      this.props.unpersistWidget()
      this.closeMenu()
    }
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutsideMenu)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutsideMenu)
  }

  toggleMenu = () => {
    if (!this.state.showMenu) {
      this.props.persistWidget()
    }

    this.setState({ showMenu: !this.state.showMenu })
  }

  closeMenu = () => {
    this.setState({ showMenu: false })
  }

  unmountWidget = () => {
    this.props.hideWidget()
    this.props.unpersistWidget()
    this.closeMenu()
  }

  closeMenuBinance = (action: any) => {
    action()
    this.closeMenu()
  }

  doTopSiteAction = (action: any) => {
    action()
    this.closeMenu()
  }

  render () {
    const {
      menuPosition,
      textDirection,
      widgetMenuPersist,
      widgetTitle,
      isForeground,
      lightWidget,
      paddingType,
      onLearnMore,
      onDisconnect,
      onRefreshData,
      onAddSite,
      onToggleCustomLinksEnabled,
      customLinksEnabled
    } = this.props
    const { showMenu } = this.state
    const hideString = widgetTitle ? `${getLocale('hide')} ${widgetTitle}` : getLocale('hide')

    return (
      <StyledWidgetMenuContainer ref={this.settingsMenuRef} paddingType={paddingType}>
        <StyledEllipsis widgetMenuPersist={widgetMenuPersist} isForeground={isForeground}>
          <IconButton isClickMenu={true} onClick={this.toggleMenu}>
            <EllipsisIcon lightWidget={lightWidget} />
          </IconButton>
        </StyledEllipsis>
        {showMenu && <StyledWidgetMenu
          textDirection={textDirection}
          menuPosition={menuPosition}
          widgetMenuPersist={widgetMenuPersist}
        >
          {
            onLearnMore
            ? <StyledWidgetLink
                onClick={this.closeMenuBinance.bind(this, onLearnMore)}
            >
              <StyledWidgetIcon><LearnMoreIcon/></StyledWidgetIcon>
              <StyledSpan>{getLocale('learnMore')}</StyledSpan>
            </StyledWidgetLink>
            : null
          }
          {
            onRefreshData
            ? <StyledWidgetButton onClick={this.closeMenuBinance.bind(this, onRefreshData)}>
                <StyledWidgetIcon isBinance={true} isRefresh={true}>
                  <RefreshIcon/>
                </StyledWidgetIcon>
                <StyledSpan>
                  {getLocale('binanceWidgetRefreshData')}
                </StyledSpan>
              </StyledWidgetButton>
            : null
          }
          {
            onDisconnect
            ? <StyledWidgetButton onClick={this.closeMenuBinance.bind(this, onDisconnect)}>
                <StyledWidgetIcon isBinance={true}>
                  <DisconnectIcon/>
                </StyledWidgetIcon>
                <StyledSpan>
                  {getLocale('binanceWidgetDisconnectButton')}
                </StyledSpan>
              </StyledWidgetButton>
            : null
          }
          {
            onAddSite
            ? <StyledWidgetButton onClick={this.doTopSiteAction.bind(this, onAddSite)}>
                <StyledWidgetIcon><AddSiteIcon/></StyledWidgetIcon>
                <StyledSpan>{getLocale('addSiteMenuLabel')}</StyledSpan>
              </StyledWidgetButton>
            : null
          }
          {
            onToggleCustomLinksEnabled
            ? <StyledWidgetButton onClick={this.doTopSiteAction.bind(this, onToggleCustomLinksEnabled)}>
                <StyledWidgetIcon>
                  {customLinksEnabled ? <FrecencyIcon/> : <FavoritesIcon/>}
                </StyledWidgetIcon>
                <StyledSpan>
                  {customLinksEnabled ? getLocale('showFrecencyMenuLabel')
                                      : getLocale('showFavoritesMenuLabel')}
                </StyledSpan>
              </StyledWidgetButton>
            : null
          }
          <StyledWidgetButton
            onClick={this.unmountWidget}
          >
            <StyledWidgetIcon><HideIcon/></StyledWidgetIcon>
            <StyledSpan>{hideString}</StyledSpan>
          </StyledWidgetButton>
        </StyledWidgetMenu>}
      </StyledWidgetMenuContainer>
    )
  }
}
