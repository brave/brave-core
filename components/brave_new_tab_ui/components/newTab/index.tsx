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
import Stats from './stats'
import Block from './block'
import FooterInfo from './footerInfo'
import SiteRemovalNotification from './notification'

interface Props {
  actions: any
  newTabData: NewTab.State
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

  render () {
    const { newTabData, actions } = this.props

    if (!newTabData || !newTabData.backgroundImage) {
      return null
    }

    return (
      <DynamicBackground background={newTabData.backgroundImage.source}>
        <Gradient />
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
          <Footer>
            <FooterInfo backgroundImageInfo={newTabData.backgroundImage} />
          </Footer>
        </Page>
      </DynamicBackground>
    )
  }
}

export default DragDropContext(HTML5Backend)(NewTabPage)
