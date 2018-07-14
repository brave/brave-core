/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

class FooterInfo extends React.Component {
  render () {
    return <footer className='footerContainer'>
      <div className='copyrightNotice'>
        {
          this.props.backgroundImage && this.props.backgroundImage.name
          ? <div>
            <div className='copyrightCredits'>
              <span className='photoBy' data-l10n-id='photoBy' /> <a className='copyrightOwner' href={this.props.backgroundImage.link} rel='noopener' target='_blank'>{this.props.backgroundImage.author}</a>
            </div>
            <span className='photoName'>{this.props.backgroundImage.name}</span>
          </div>
          : null
        }
      </div>
      <nav className='shortcutsContainer'>
        <a href='chrome://settings'><span className='shortcutIcon settingsIcon' data-l10n-id='preferencesPage' /></a>
        <a href='chrome://bookmarks'><span className='shortcutIcon bookmarksIcon' data-l10n-id='bookmarksPage' /></a>
        <a href='chrome://history'><span className='shortcutIcon historyIcon' data-l10n-id='historyPage' /></a>
      </nav>
    </footer>
  }
}

module.exports = FooterInfo
