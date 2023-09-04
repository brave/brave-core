// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StyledWidgetMenuContainer, StyledWidgetMenu, StyledWidgetButton, StyledWidgetIcon, StyledSpan, StyledWidgetLink, StyledEllipsis } from './styles'
import { IconButton } from '../../default'
import EllipsisIcon from '../../popupMenu/ellipsisIcon'
import HideIcon from './assets/hide'
import AddSiteIcon from './assets/add-site'
import FrecencyIcon from './assets/frecency'
import LearnMoreIcon from './assets/learn-more'
import FavoritesIcon from './assets/favorites'
import { getLocale } from '../../../../common/locale'

export interface WidgetMenuCustomItem {
  label: string
  renderIcon: () => React.ReactNode
  onClick: () => void
}

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
  onAddSite?: () => void
  customLinksEnabled?: boolean
  onToggleCustomLinksEnabled?: () => void
  customMenuItems?: WidgetMenuCustomItem[]
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

  closeMenuWidget = (action: any) => {
    action()
    this.closeMenu()
  }

  doTopSiteAction = (action: any) => {
    action()
    this.closeMenu()
  }

  renderCustomMenuItems () {
    const { customMenuItems } = this.props

    if (!customMenuItems) {
      return null
    }

    return (
      <>
        {
          customMenuItems.map((item, index) => {
            return (
              <StyledWidgetLink
                key={index}
                onClick={this.closeMenuWidget.bind(this, item.onClick)}
              >
                <StyledWidgetIcon>{item.renderIcon()}</StyledWidgetIcon>
                <StyledSpan>{getLocale(item.label)}</StyledSpan>
              </StyledWidgetLink>
            )
          })
        }
      </>
    )
  }

  render () {
    const {
      menuPosition,
      textDirection,
      widgetMenuPersist,
      widgetTitle,
      isForeground,
      paddingType,
      onLearnMore,
      onAddSite,
      onToggleCustomLinksEnabled,
      customLinksEnabled,
      customMenuItems
    } = this.props
    const { showMenu } = this.state
    const hideString = widgetTitle ? `${getLocale('hide')} ${widgetTitle}` : getLocale('hide')

    return (
      <StyledWidgetMenuContainer ref={this.settingsMenuRef} paddingType={paddingType}>
        <StyledEllipsis widgetMenuPersist={widgetMenuPersist} isForeground={isForeground}>
          <IconButton isClickMenu={true} onClick={this.toggleMenu}>
            <EllipsisIcon />
          </IconButton>
        </StyledEllipsis>
        {showMenu && <StyledWidgetMenu
          textDirection={textDirection}
          menuPosition={menuPosition}
          widgetMenuPersist={widgetMenuPersist}
        >
          { customMenuItems ? this.renderCustomMenuItems() : null }
          {
            onLearnMore
            ? <StyledWidgetLink
                onClick={this.closeMenuWidget.bind(this, onLearnMore)}
            >
              <StyledWidgetIcon><LearnMoreIcon/></StyledWidgetIcon>
              <StyledSpan>{getLocale('learnMore')}</StyledSpan>
            </StyledWidgetLink>
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
