// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StyledWidgetMenuContainer, StyledWidgetMenu, StyledWidgetButton, StyledWidgetIcon, StyledSpan, StyledWidgetLink } from './styles'
import { IconButton } from '../../default'
import EllipsisIcon from './assets/ellipsis'
import HideIcon from './assets/hide'
import LearnMoreIcon from './assets/learn-more'
import { getLocale } from '../../../../common/locale'

interface Props {
  menuPosition: 'right' | 'left'
  hideWidget: () => void
  textDirection: string
  toggleWidgetHover: () => void
  widgetMenuPersist: boolean
  unpersistWidgetHover: () => void
  widgetTitle?: string
  onLearnMore?: () => void
  onMouseEnter: () => void
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
      this.props.unpersistWidgetHover()
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
    this.setState({ showMenu: !this.state.showMenu })
  }

  closeMenu = () => {
    this.setState({ showMenu: false })
  }

  unmountWidget = () => {
    this.props.hideWidget()
    this.props.unpersistWidgetHover()
    this.closeMenu()
  }

  render () {
    const { menuPosition, textDirection, widgetMenuPersist, widgetTitle, onLearnMore, onMouseEnter } = this.props
    const { showMenu } = this.state
    const hideString = widgetTitle ? `${getLocale('hide')} ${widgetTitle}` : getLocale('hide')

    return (
      <StyledWidgetMenuContainer
        menuPosition={menuPosition}
        innerRef={this.settingsMenuRef}
        widgetMenuPersist={widgetMenuPersist}
        onMouseEnter={onMouseEnter}
      >
        <IconButton isClickMenu={true} onClick={this.toggleMenu}><EllipsisIcon/></IconButton>
        {showMenu && <StyledWidgetMenu
          textDirection={textDirection}
          menuPosition={menuPosition}
        >
          <StyledWidgetButton
            onClick={this.unmountWidget}
          >
            <StyledWidgetIcon><HideIcon/></StyledWidgetIcon>
            <StyledSpan>{hideString}</StyledSpan>
          </StyledWidgetButton>
          {
            onLearnMore
            ? <StyledWidgetLink
                onClick={onLearnMore}
            >
              <StyledWidgetIcon><LearnMoreIcon/></StyledWidgetIcon>
              <StyledSpan>{`${getLocale('learnMore')}`}</StyledSpan>
            </StyledWidgetLink>
            : null
          }
        </StyledWidgetMenu>}
      </StyledWidgetMenuContainer>
    )
  }
}
