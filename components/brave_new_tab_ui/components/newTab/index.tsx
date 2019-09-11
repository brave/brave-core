/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { DragDropContext } from 'react-dnd'
import HTML5Backend from 'react-dnd-html5-backend'
import {
  Page,
  Header,
  Clock,
  Main,
  List,
  Footer,
  DynamicBackground,
  Gradient
} from 'brave-ui/features/newTab/default'

// Components
import Settings from './settings'
import Stats from './stats'
import Block from './block'
import FooterInfo from './footerInfo'
import SiteRemovalNotification from './notification'

interface Props {
  newTabData: NewTab.State
  actions: any
  saveShowBackgroundImage: (value: boolean) => void
}

class NewTabPage extends React.Component<Props, {}> {
  componentDidMount () {
    // if a notification is open at component mounting time, close it
    this.props.actions.onHideSiteRemovalNotification()
  }

  onDraggedSite = (fromUrl: string, toUrl: string, dragRight: boolean) => {
    this.props.actions.siteDragged(fromUrl, toUrl, dragRight)
  }

  onDragEnd = (url: string, didDrop: boolean) => {
    this.props.actions.siteDragEnd(url, didDrop)
  }

  onToggleBookmark (site: NewTab.Site) {
    if (site.bookmarked === undefined) {
      this.props.actions.bookmarkAdded(site.url)
    } else {
      this.props.actions.bookmarkRemoved(site.url)
    }
  }

  onTogglePinnedTopSite (site: NewTab.Site) {
    if (!site.pinned) {
      this.props.actions.sitePinned(site.url)
    } else {
      this.props.actions.siteUnpinned(site.url)
    }
  }

  onIgnoredTopSite (site: NewTab.Site) {
    this.props.actions.siteIgnored(site.url)
  }

  toggleShowBackgroundImage = () => {
    this.props.saveShowBackgroundImage(
      !this.props.newTabData.showBackgroundImage
    )
  }

  showSettings = () => {
    this.props.actions.showSettingsMenu()
  }

  closeSettings = () => {
    this.props.actions.closeSettingsMenu()
  }

  render () {
    const { newTabData, actions } = this.props

    if (!newTabData) {
      return null
    }

    return (
      <DynamicBackground showBackgroundImage={newTabData.showBackgroundImage} background={newTabData.backgroundImage ? newTabData.backgroundImage.source : ''}>
        {newTabData.showBackgroundImage && <Gradient />}
        <Page>
          <Header>
            <Stats stats={newTabData.stats} />
            <Clock />
            <Main>
              <List>
                {
                  this.props.newTabData.gridSites.map((site: NewTab.Site) =>
                    <Block
                      key={site.url}
                      id={site.url}
                      title={site.title}
                      href={site.url}
                      favicon={site.favicon}
                      style={{ backgroundColor: site.themeColor || site.computedThemeColor }}
                      onToggleBookmark={this.onToggleBookmark.bind(this, site)}
                      onPinnedTopSite={this.onTogglePinnedTopSite.bind(this, site)}
                      onIgnoredTopSite={this.onIgnoredTopSite.bind(this, site)}
                      onDraggedSite={this.onDraggedSite}
                      onDragEnd={this.onDragEnd}
                      isPinned={site.pinned}
                      isBookmarked={site.bookmarked !== undefined}
                    />
                  )
                }
              </List>
              {
                this.props.newTabData.showSiteRemovalNotification
                ? <SiteRemovalNotification actions={actions} />
                : null
              }
            </Main>
          </Header>
          {
            newTabData.showSettings &&
            <Settings
              onClickOutside={this.closeSettings}
              toggleShowBackgroundImage={this.toggleShowBackgroundImage}
              showBackgroundImage={newTabData.showBackgroundImage}
            />
          }
          <Footer>
            <FooterInfo
              backgroundImageInfo={newTabData.backgroundImage}
              onClickSettings={this.showSettings}
              isSettingsMenuOpen={newTabData.showSettings}
              showPhotoInfo={newTabData.showBackgroundImage}
            />
          </Footer>
        </Page>
      </DynamicBackground>
    )
  }
}

export default DragDropContext(HTML5Backend)(NewTabPage)
