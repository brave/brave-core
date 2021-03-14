// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { SortableElement, SortableElementProps } from 'react-sortable-hoc'

// Feature-specific components
import {
  Tile,
  TileActionsContainer,
  TileAction,
  TileFavicon,
  TileMenu,
  TileMenuItem,
  TileTitle
} from '../../components/default'

// Icons
import EditIcon from '../../components/default/gridSites/assets/edit'
import EditMenuIcon from '../../components/default/gridSites/assets/edit-menu'
import TrashIcon from '../../components/default/gridSites/assets/trash'

// Types
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'

import { getLocale } from '../../../common/locale'

interface Props {
  actions: typeof newTabActions & typeof gridSitesActions
  siteData: NewTab.Site
  disabled: boolean
  isDragging: boolean
  onShowEditTopSite: (targetTopSiteForEditing?: NewTab.Site) => void
}

interface State {
  showMenu: boolean
}

function generateGridSiteFavicon (site: NewTab.Site): string {
  if (site.favicon === '') {
    return `chrome://favicon/size/64@1x/${site.url}`
  }
  return site.favicon
}

class TopSite extends React.PureComponent<Props, State> {
  tileMenuRef: React.RefObject<any>
  constructor (props: Props) {
    super(props)
    this.state = {
      showMenu: false
    }
    this.tileMenuRef = React.createRef()
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutside)
    document.addEventListener('mouseMove', this.handleMoveOutside)
    document.addEventListener('keydown', this.onKeyPressSettings)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutside)
    document.removeEventListener('mousemove', this.handleMoveOutside)
    document.removeEventListener('keydown', this.onKeyPressSettings)
  }

  onKeyPressSettings = (event: KeyboardEvent) => {
    if (event.key === 'Escape') {
      this.setState({ showMenu: false })
    }
  }

  handleClickOutside = (event: Event) => {
    if (
      this.tileMenuRef &&
      this.tileMenuRef.current &&
      !this.tileMenuRef.current.contains(event.target)
    ) {
      this.setState({ showMenu: false })
    }
  }

  handleMoveOutside = (event: Event) => {
    if (
      this.tileMenuRef &&
      this.tileMenuRef.current &&
      !this.tileMenuRef.current.contains(event.target)
    ) {
      this.setState({ showMenu: false })
    }
  }

  onIgnoredTopSite (site: NewTab.Site, e: Event) {
    e.preventDefault()
    this.setState({ showMenu: false })
    this.props.actions.tileRemoved(site.url)
  }

  onEditTopSite (site: NewTab.Site, e: Event) {
    e.preventDefault()
    this.setState({ showMenu: false })
    this.props.onShowEditTopSite(site)
  }

  onShowTileMenu = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
    e.preventDefault()
    this.setState({ showMenu: true })
  }

  render () {
    const { siteData, isDragging } = this.props
    return (
      <Tile
        title={siteData.title}
        tabIndex={0}
        isDragging={isDragging}
        isMenuShowing={this.state.showMenu}
        href={siteData.url}
      >
        {
          !siteData.defaultSRTopSite
          ? <TileActionsContainer>
              <TileAction onClick={this.onShowTileMenu}>
                <EditIcon/>
              </TileAction>
            </TileActionsContainer>
          : null
        }
        { this.state.showMenu &&
          <TileMenu ref={this.tileMenuRef}>
            <TileMenuItem onClick={this.onEditTopSite.bind(this, siteData)}>
              <EditMenuIcon />
              {getLocale('editSiteTileMenuItem')}
            </TileMenuItem>
            <TileMenuItem onClick={this.onIgnoredTopSite.bind(this, siteData)}>
              <TrashIcon />
              {getLocale('removeTileMenuItem')}
            </TileMenuItem>
          </TileMenu>
        }
        <TileFavicon
          draggable={false}
          src={generateGridSiteFavicon(siteData)}
        />
        <TileTitle> {siteData.title} </TileTitle>
      </Tile>
    )
  }
}

type TopSiteSortableElementProps = SortableElementProps & Props
export default SortableElement(
  (props: TopSiteSortableElementProps) => <TopSite {...props} />
)
