/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const locale: any = {
  // stats
  trackersBlocked: 'Trackers Blocked',
  adsBlocked: 'Ads Blocked',
  httpsUpgrades: 'HTTPS Upgrades',
  estimatedTimeSaved: 'Estimated Time Saved',
  minutes: 'minutes',
  // footer info
  photoBy: 'Photo by',
  // site removal notification
  thumbRemoved: 'Top site removed',
  undoRemoved: 'Undo',
  restoreAll: 'Restore All',
  close: 'Close',
  // settings
  dashboardSettings: 'Dashboard Settings',
  showBackgroundImg: 'Show background image',
  showBraveStats: 'Show Brave Stats',
  showClock: 'Show Clock',
  showTopSites: 'Show Top Sites'
}

export default locale

export const getLocale = (word: string) => locale[word]
