/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import deepFreeze from 'deep-freeze-node'
import * as shieldsPanelState from '../../../app/state/shieldsPanelState'

const state = deepFreeze({
  currentWindowId: 1,
  tabs: {
    2: {
      id: 2,
      adBlock: 'block',
      trackingProtection: 'block',
      httpsEverywhere: 'block',
      javascript: 'block'
    },
    3: {
      id: 3
    },
    4: {
      id: 4
    }
  },
  windows: {1: 2, 2: 3}
})

describe('shieldsPanelState test', () => {
  describe('getActiveTabId', () => {
    it('Obtains the active tab ID based on the current window', function () {
      assert.equal(shieldsPanelState.getActiveTabId(state), 2)
    })
  })
  describe('getActiveTabData', () => {
    it('', function () {
      assert.deepEqual(shieldsPanelState.getActiveTabData(state), {
        id: 2,
        adBlock: 'block',
        trackingProtection: 'block',
        httpsEverywhere: 'block',
        javascript: 'block'
      })
    })
  })
  describe('updateActiveTab', () => {
    it('can update focused window', function () {
      assert.deepEqual(shieldsPanelState.updateActiveTab(state, 1, 4), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 4, 2: 3}
      })
    })
    it('can update a window which is not focused', function () {
      assert.deepEqual(shieldsPanelState.updateActiveTab(state, 2, 4), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 4}
      })
    })
  })
  describe('removeWindowInfo', () => {
    it('can remove the focused window', function () {
      assert.deepEqual(shieldsPanelState.removeWindowInfo(state, 1), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {2: 3}
      })
    })
    it('can remove a window which is not focused', function () {
      assert.deepEqual(shieldsPanelState.removeWindowInfo(state, 2), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2}
      })
    })
  })
  describe('updateFocusedWindow', () => {
    it('updates the currentWindowId', function () {
      assert.deepEqual(shieldsPanelState.updateFocusedWindow(state, 2), {
        currentWindowId: 2,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 3}
      })
    })
  })
  describe('updateTabShieldsData', () => {
    it('updates the correct tabId data', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateTabShieldsData(state, this.tabId, {
        adBlock: 'allow',
        trackingProtection: 'allow',
        httpsEverywhere: 'allow',
        javascript: 'allow'
      }), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'allow',
            trackingProtection: 'allow',
            httpsEverywhere: 'allow',
            javascript: 'allow',
            adsTrackers: 'allow',
            controlsOpen: true,
            shieldsEnabled: 'allow',
            adsBlocked: 0,
            trackingProtectionBlocked: 0,
            httpsEverywhereRedirected: 0,
            javascriptBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 3}
      })
    })
  })
  describe('resetBlockingStats', () => {
    it('sets the specified stats back to 0 for the active tab', function () {
      this.tabId = 2
      const stateWithStats = {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block',
            adsBlocked: 3,
            trackingProtectionBlocked: 4444,
            httpsEverywhereRedirected: 5,
            javascriptBlocked: 5
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 3}
      }

      stateWithStats.tabs[this.tabId].adsBlocked = 3
      stateWithStats.tabs[this.tabId].trackingProtectionBlocked = 4
      stateWithStats.tabs[this.tabId].httpsEverywhereRedirected = 5
      stateWithStats.tabs[this.tabId].javascriptBlocked = 6
      assert.deepEqual(shieldsPanelState.resetBlockingStats(stateWithStats, this.tabId), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block',
            adsBlocked: 0,
            trackingProtectionBlocked: 0,
            httpsEverywhereRedirected: 0,
            javascriptBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 3}
      })
    })
    it('sets the specified stats back to 0 for the a non active tab', function () {
      this.tabId = 4
      const stateWithStats = {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4,
            adsBlocked: 3,
            trackingProtectionBlocked: 4444,
            httpsEverywhereRedirected: 5,
            javascriptBlocked: 5
          }
        },
        windows: {1: 2, 2: 3}
      }

      assert.deepEqual(shieldsPanelState.resetBlockingStats(stateWithStats, this.tabId), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4,
            adsBlocked: 0,
            trackingProtectionBlocked: 0,
            httpsEverywhereRedirected: 0,
            javascriptBlocked: 0
          }
        },
        windows: {1: 2, 2: 3}
      })
    })
  })
  describe('updateResourceBlocked', () => {
    it('can update ads blocked count', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'adBlock'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block',
            adsBlocked: 1,
            trackingProtectionBlocked: 0,
            httpsEverywhereRedirected: 0,
            javascriptBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 3}
      })
    })
    it('can update tracking protection blocked count', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'adBlock'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'block',
            trackingProtection: 'block',
            httpsEverywhere: 'block',
            javascript: 'block',
            adsBlocked: 1,
            trackingProtectionBlocked: 0,
            httpsEverywhereRedirected: 0,
            javascriptBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {1: 2, 2: 3}
      })
    })
  })
})
