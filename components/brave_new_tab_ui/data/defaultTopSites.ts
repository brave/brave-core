/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import AppStoreFavicon from '../../img/newtab/defaultTopSitesIcon/appstore.png'
import BraveFavicon from '../../img/newtab/defaultTopSitesIcon/brave.png'
import FacebookFavicon from '../../img/newtab/defaultTopSitesIcon/facebook.png'
import PlayStoreFavicon from '../../img/newtab/defaultTopSitesIcon/playstore.png'
import TwitterFavicon from '../../img/newtab/defaultTopSitesIcon/twitter.png'
import YouTubeFavicon from '../../img/newtab/defaultTopSitesIcon/youtube.png'

export const defaultTopSitesData = [
  {
    name: 'App Store',
    url: 'https://itunes.apple.com/app/brave-browser-fast-adblocker/id1052879175?mt=8',
    favicon: AppStoreFavicon,
    background: 'rgba(255,255,255,0.8)'
  },
  {
    name: 'Brave Software',
    url: 'https://brave.com',
    favicon: BraveFavicon,
    background: 'rgba(255,255,255,0.8)'
  },
  {
    name: 'Facebook',
    url: 'https://www.facebook.com/BraveSoftware/',
    favicon: FacebookFavicon,
    background: 'rgba(255,255,255,0.8)'
  },
  {
    name: 'Play Store',
    url: 'https://play.google.com/store/apps/details?id=com.brave.browser&hl=en_US',
    favicon: PlayStoreFavicon,
    background: 'rgba(255,255,255,0.8)'
  },
  {
    name: 'Twitter',
    url: 'https://twitter.com/brave',
    favicon: TwitterFavicon,
    background: 'rgba(255,255,255,0.8)'
  },
  {
    name: 'YouTube',
    url: 'https://www.youtube.com/bravesoftware',
    favicon: YouTubeFavicon,
    background: 'rgba(255,255,255,0.8)'
  }
]
