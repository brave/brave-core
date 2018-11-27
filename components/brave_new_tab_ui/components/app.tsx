/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { DragDropContext } from 'react-dnd'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import HTML5Backend from 'react-dnd-html5-backend'
import { Grid, Column } from 'brave-ui/components'
import { Clock } from 'brave-ui/old'

// Components
import Stats from './stats'
import Block from './block'
import FooterInfo from './footerInfo'
import SiteRemovalNotification from './siteRemovalNotification'
import NewPrivateTab from './privateTab'

// Constants
import { theme } from '../constants/theme'

// Utils
import * as newTabActions from '../actions/new_tab_actions'

// Assets
require('../../styles/newtab.less')
require('font-awesome/css/font-awesome.css')
require('../../fonts/poppins.css')
require('../../fonts/muli.css')

interface Props {
  actions: any
  newTabData: NewTab.State
}

class NewTabPage extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  get showImages () {
    return this.props.newTabData.showImages && !!this.props.newTabData.backgroundImage
  }

  get useAlternativePrivateSearchEngine () {
    return (
      this.props.newTabData &&
      this.props.newTabData.useAlternativePrivateSearchEngine
    )
  }

  onDraggedSite = (fromUrl: string, toUrl: string, dragRight: boolean) => {
    this.actions.siteDragged(fromUrl, toUrl, dragRight)
  }

  onDragEnd = (url: string, didDrop: boolean) => {
    this.actions.siteDragEnd(url, didDrop)
  }

  onToggleBookmark (site: NewTab.Site) {
    if (site.bookmarked === undefined) {
      this.actions.bookmarkAdded(site.url)
    } else {
      this.actions.bookmarkRemoved(site.url)
    }
  }

  onHideSiteRemovalNotification = () => {
    this.actions.onHideSiteRemovalNotification()
  }

  onTogglePinnedTopSite (site: NewTab.Site) {
    if (!site.pinned) {
      this.actions.sitePinned(site.url)
    } else {
      this.actions.siteUnpinned(site.url)
    }
  }

  onIgnoredTopSite (site: NewTab.Site) {
    this.actions.siteIgnored(site.url)
  }

  onUndoIgnoredTopSite = () => {
    this.actions.undoSiteIgnored()
  }

  /**
   * Clear ignoredTopSites and pinnedTopSites list
   */
  onUndoAllSiteIgnored = () => {
    this.actions.undoAllSiteIgnored()
  }

  /**
   * This handler only fires when the image fails to load.
   * If both the remote and local image fail, page defaults to gradients.
   */
  onBackgroundImageLoadFailed = () => {
    this.actions.backgroundImageLoadFailed()
  }

  onChangePrivateSearchEngine = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (!e.target) {
      return
    }
    this.actions.changePrivateSearchEngine(e.target.checked)
  }

  render () {
    const { newTabData } = this.props

    // don't render if user prefers an empty page
    if (this.props.newTabData.showEmptyPage && !this.props.newTabData.isIncognito) {
      return <div className='empty' />
    }

    if (this.props.newTabData.isIncognito) {
      return (
        <NewPrivateTab
          useAlternativePrivateSearchEngine={this.useAlternativePrivateSearchEngine}
          isTor={this.props.newTabData.isTor}
          isQwant={this.props.newTabData.isQwant}
          onChangePrivateSearchEngine={this.onChangePrivateSearchEngine}
        />
      )
    }

    // don't render until object is found
    if (!this.props.newTabData) {
      return null
    }

    const backgroundProps: Partial<NewTab.Image> = {}
    const bgImage: NewTab.Image | undefined = this.props.newTabData.backgroundImage
    let gradientClassName = 'gradient'
    if (this.showImages && bgImage) {
      backgroundProps.style = bgImage.style
      gradientClassName = 'bgGradient'
    }
    return (
      <div data-test-id='dynamicBackground' className='dynamicBackground' {...backgroundProps}>
        {
          this.showImages && bgImage
            ? <img src={bgImage.source} onError={this.onBackgroundImageLoadFailed} data-test-id='backgroundImage' />
            : null
        }
        <div data-test-id={this.showImages ? 'bgGradient' : 'gradient'} className={gradientClassName} />
        <div className='content'>
          <main style={theme.newTab}>
            <div style={theme.newTabStats}>
              <Grid columns={3}>
                <Column size={2}>
                  <Stats stats={newTabData.stats}/>
                </Column>
                <Column size={1} customStyle={theme.clockContainer}>
                  <Clock customStyle={theme.clock} />
                </Column>
              </Grid>
            </div>
            <div className='topSitesContainer'>
              <nav className='topSitesGrid'>
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
              </nav>
            </div>
          </main>
          {
            this.props.newTabData.showSiteRemovalNotification
              ? <SiteRemovalNotification
                onUndoIgnoredTopSite={this.onUndoIgnoredTopSite}
                onRestoreAll={this.onUndoAllSiteIgnored}
                onCloseNotification={this.onHideSiteRemovalNotification}
              />
              : null
          }
          <FooterInfo backgroundImage={bgImage} />
        </div>
      </div>
    )
  }
}

const mapStateToProps = (state: NewTab.ApplicationState) => ({
  newTabData: state.newTabData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(newTabActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(DragDropContext(HTML5Backend)(NewTabPage))
