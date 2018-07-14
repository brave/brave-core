/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
const { DragDropContext } = require('react-dnd')
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const HTML5Backend = require('react-dnd-html5-backend')
const newTabActions = require('../actions/newTabActions')

// Components
const { Grid, Column, Clock } = require('brave-ui')
const Stats = require('./stats')
const Block = require('./block')
const FooterInfo = require('./footerInfo')
const SiteRemovalNotification = require('./siteRemovalNotification')
const NewPrivateTab = require('./newPrivateTab')

// Assets
const theme = require('./theme')
require('../../styles/newtab.less')
require('font-awesome/css/font-awesome.css')
require('../../fonts/poppins.css')

class NewTabPage extends React.Component {
  constructor (props) {
    super(props)
    this.onBackgroundImageLoadFailed = this.onBackgroundImageLoadFailed.bind(this)
    this.onUndoAllSiteIgnored = this.onUndoAllSiteIgnored.bind(this)
    this.onUndoIgnoredTopSite = this.onUndoIgnoredTopSite.bind(this)
    this.onHideSiteRemovalNotification = this.onHideSiteRemovalNotification.bind(this)
    this.onDraggedSite = this.onDraggedSite.bind(this)
    this.onDragEnd = this.onDragEnd.bind(this)
    this.onChangePrivateSearchEngine = this.onChangePrivateSearchEngine.bind(this)
  }

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

  onDraggedSite (fromUrl, toUrl, dragRight) {
    this.actions.siteDragged(fromUrl, toUrl, dragRight)
  }

  onDragEnd (url, didDrop) {
    this.actions.siteDragEnd(url, didDrop)
  }

  onToggleBookmark (site) {
    if (site.bookmarked) {
      this.actions.bookmarkAdded(site.url)
    } else {
      this.actions.bookmarkRemoved(site.url)
    }
  }

  onHideSiteRemovalNotification () {
    this.actions.onHideSiteRemovalNotification()
  }

  onTogglePinnedTopSite (site) {
    if (!site.pinned) {
      this.actions.sitePinned(site.url)
    } else {
      this.actions.siteUnpinned(site.url)
    }
  }

  onIgnoredTopSite (site) {
    this.actions.siteIgnored(site.url)
  }

  onUndoIgnoredTopSite () {
    this.actions.undoSiteIgnored()
  }

  /**
   * Clear ignoredTopSites and pinnedTopSites list
   */
  onUndoAllSiteIgnored () {
    this.actions.undoAllSiteIgnored()
  }

  /**
   * This handler only fires when the image fails to load.
   * If both the remote and local image fail, page defaults to gradients.
   */
  onBackgroundImageLoadFailed () {
    this.actions.backgroundImageLoadFailed()
  }

  onChangePrivateSearchEngine (e) {
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
          stats={newTabData.stats}
          useAlternativePrivateSearchEngine={this.useAlternativePrivateSearchEngine}
          onChangePrivateSearchEngine={this.onChangePrivateSearchEngine}
        />
      )
    }

    // don't render until object is found
    if (!this.props.newTabData) {
      return null
    }

    const backgroundProps = {}
    let gradientClassName = 'gradient'
    if (this.showImages) {
      backgroundProps.style = this.props.newTabData.backgroundImage.style
      gradientClassName = 'bgGradient'
    }
    return <div data-test-id='dynamicBackground' className='dynamicBackground' {...backgroundProps}>
      {
        this.showImages
          ? <img src={this.props.newTabData.backgroundImage.source} onError={this.onBackgroundImageLoadFailed} data-test-id='backgroundImage' />
          : null
      }
      <div data-test-id={this.showImages ? 'bgGradient' : 'gradient'} className={gradientClassName} />
      <div className='content'>
        <main style={theme.newTab}>
          <div style={theme.newTabStats}>
            <Grid theme={theme.stats} columns={3}>
              <Column size={2}>
                <Stats stats={newTabData.stats} theme={theme} />
              </Column>
              <Column size={1} theme={theme.clockContainer}>
                <Clock theme={theme.clock} />
              </Column>
            </Grid>
          </div>
          <div className='topSitesContainer'>
            <nav className='topSitesGrid'>
              {
                this.props.newTabData.gridSites.map((site, i) =>
                  <Block
                    key={site.url}
                    id={site.url}
                    title={site.title}
                    href={site.url}
                    favicon={site.favicon}
                    letter={site.letter}
                    thumb={site.thumb}
                    style={{ backgroundColor: site.themeColor || site.computedThemeColor }}
                    onToggleBookmark={this.onToggleBookmark.bind(this, site)}
                    onPinnedTopSite={this.onTogglePinnedTopSite.bind(this, site)}
                    onIgnoredTopSite={this.onIgnoredTopSite.bind(this, site)}
                    onDraggedSite={this.onDraggedSite}
                    onDragEnd={this.onDragEnd}
                    isPinned={site.pinned}
                    isBookmarked={site.bookmarked}
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
        {
          <FooterInfo backgroundImage={this.props.newTabData.backgroundImage} />}
      </div>
    </div>
  }
}

const mapStateToProps = (state) => ({
  newTabData: state.newTabData
})

const mapDispatchToProps = (dispatch) => ({
  actions: bindActionCreators(newTabActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(DragDropContext(HTML5Backend)(NewTabPage))
