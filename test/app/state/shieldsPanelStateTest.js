/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import * as shieldsPanelState from '../../../app/state/shieldsPanelState'

const state = {
  currentWindowId: 1,
  tabs: {
    2: {
      id: 2,
      adBlock: 'block',
      trackingProtection: 'block'
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
        trackingProtection: 'block'
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
            trackingProtection: 'block'
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
            trackingProtection: 'block'
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
            trackingProtection: 'block'
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
            trackingProtection: 'block'
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
            trackingProtection: 'block'
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
        trackingProtection: 'allow'
      }), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            adBlock: 'allow',
            trackingProtection: 'allow',
            adsBlocked: 0,
            trackingProtectionBlocked: 0
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
            adsBlocked: 1,
            trackingProtectionBlocked: 0
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
            adsBlocked: 1,
            trackingProtectionBlocked: 0
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
