/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { DragDropContext } from 'react-dnd'
import HTML5Backend from 'react-dnd-html5-backend'
import {
  Page,
  Header,
  ClockWidget as Clock,
  ListWidget as List,
  Footer,
  App,
  PosterBackground,
  Gradient
} from '../../components/default'

// Components
import Stats from './stats'
import Block from './block'
import FooterInfo from './footerInfo'
import SiteRemovalNotification from './notification'

interface Props {
  newTabData: NewTab.State
  actions: any
  saveShowBackgroundImage: (value: boolean) => void
  saveShowClock: (value: boolean) => void
  saveShowTopSites: (value: boolean) => void
  saveShowStats: (value: boolean) => void
}

interface State {
  showSettingsMenu: boolean
  backgroundHasLoaded: boolean
}

class NewTabPage extends React.Component<Props, State> {
  state = {
    showSettingsMenu: false,
    backgroundHasLoaded: false
  }

  componentDidMount () {
    // if a notification is open at component mounting time, close it
    this.props.actions.onHideSiteRemovalNotification()
    this.trackCachedImage()
  }

  componentDidUpdate (prevProps: Props) {
    if (!prevProps.newTabData.showBackgroundImage &&
          this.props.newTabData.showBackgroundImage) {
      this.trackCachedImage()
    }
    if (prevProps.newTabData.showBackgroundImage &&
      !this.props.newTabData.showBackgroundImage) {
      // reset loaded state
      this.setState({ backgroundHasLoaded: false })
    }
  }

  trackCachedImage () {
    if (this.props.newTabData.showBackgroundImage &&
        this.props.newTabData.backgroundImage &&
        this.props.newTabData.backgroundImage.source) {
      const imgCache = new Image()
      imgCache.src = this.props.newTabData.backgroundImage.source
      console.timeStamp('image start loading...')
      imgCache.onload = () => {
        console.timeStamp('image loaded')
        this.setState({
          backgroundHasLoaded: true
        })
      }
    }
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

  toggleShowClock = () => {
    this.props.saveShowClock(
      !this.props.newTabData.showClock
    )
  }

  toggleShowStats = () => {
    this.props.saveShowStats(
      !this.props.newTabData.showStats
    )
  }

  toggleShowTopSites = () => {
    this.props.saveShowTopSites(
      !this.props.newTabData.showTopSites
    )
  }

  closeSettings = () => {
    this.setState({ showSettingsMenu: false })
  }

  toggleSettings = () => {
    this.setState({ showSettingsMenu: !this.state.showSettingsMenu })
  }

  render () {
    const { newTabData, actions } = this.props
    const { showSettingsMenu } = this.state

    if (!newTabData) {
      return null
    }

    return (
      <App dataIsReady={newTabData.initialDataLoaded}>
        <PosterBackground
          hasImage={newTabData.showBackgroundImage}
          imageHasLoaded={this.state.backgroundHasLoaded}
        >
          {newTabData.showBackgroundImage && newTabData.backgroundImage &&
            <img src={newTabData.backgroundImage.source} />
          }
        </PosterBackground>
        {newTabData.showBackgroundImage &&
          <Gradient
            imageHasLoaded={this.state.backgroundHasLoaded}
          />
        }
        <Page>
          <Header>
            <Stats
              textDirection={newTabData.textDirection}
              stats={newTabData.stats}
              showWidget={newTabData.showStats}
              hideWidget={this.toggleShowStats}
              menuPosition={'right'}
            />
            <Clock
              textDirection={newTabData.textDirection}
              showWidget={newTabData.showClock}
              hideWidget={this.toggleShowClock}
              menuPosition={'left'}
            />
            <List
              textDirection={newTabData.textDirection}
              showWidget={newTabData.showTopSites}
              menuPosition={'right'}
              hideWidget={this.toggleShowTopSites}
            >
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
          </Header>
          <Footer>
            <FooterInfo
              textDirection={newTabData.textDirection}
              onClickOutside={this.closeSettings}
              backgroundImageInfo={newTabData.backgroundImage}
              onClickSettings={this.toggleSettings}
              showSettingsMenu={showSettingsMenu}
              showPhotoInfo={newTabData.showBackgroundImage}
              toggleShowBackgroundImage={this.toggleShowBackgroundImage}
              toggleShowClock={this.toggleShowClock}
              toggleShowStats={this.toggleShowStats}
              toggleShowTopSites={this.toggleShowTopSites}
              showBackgroundImage={newTabData.showBackgroundImage}
              showClock={newTabData.showClock}
              showStats={newTabData.showStats}
              showTopSites={newTabData.showTopSites}
            />
          </Footer>
        </Page>
      </App>
    )
  }
}

export default DragDropContext(HTML5Backend)(NewTabPage)
