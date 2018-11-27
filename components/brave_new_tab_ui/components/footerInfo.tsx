/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../common/locale'

interface Props {
  backgroundImage?: NewTab.Image
}

export default class FooterInfo extends React.Component<Props, {}> {
  render () {
    const bgImage: NewTab.Image | undefined = this.props.backgroundImage

    return (
      <footer className='footerContainer'>
        <div className='copyrightNotice'>
          {
            bgImage && bgImage.name
            ? <div>
              <div className='copyrightCredits'>
                <span className='photoBy'>{getLocale('photoBy')} </span>
                <a className='copyrightOwner' href={bgImage.link} rel='noopener' target='_blank'>
                  {bgImage.author}
                </a>
              </div>
              <span className='photoName'>{bgImage.name}</span>
            </div>
            : null
          }
        </div>
        <nav className='shortcutsContainer'>
          <a href='chrome://settings'><span className='shortcutIcon settingsIcon' title={getLocale('preferencesPageTitle')} /></a>
          <a href='chrome://bookmarks'><span className='shortcutIcon bookmarksIcon' title={getLocale('bookmarksPageTitle')} /></a>
          <a href='chrome://history'><span className='shortcutIcon historyIcon' title={getLocale('historyPageTitle')} /></a>
        </nav>
      </footer>
    )
  }
}
