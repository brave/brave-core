/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as deepFreeze from 'deep-freeze-node'
import * as shieldsPanelState from '../../../app/state/shieldsPanelState'
import { State } from '../../../app/types/state/shieldsPannelState';

const state: State = deepFreeze({
  currentWindowId: 1,
  tabs: {
    2: {
      id: 2,
      ads: 'block',
      trackers: 'block',
      httpUpgradableResources: 'block',
      javascript: 'block',
      fingerprinting: 'block'
    },
    3: {
      id: 3
    },
    4: {
      id: 4
    }
  },
  windows: {
    1: 2,
    2: 3
  }
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
        ads: 'block',
        trackers: 'block',
        httpUpgradableResources: 'block',
        javascript: 'block',
        fingerprinting: 'block'
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
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 4,
          2: 3
        }
      })
    })
    it('can update a window which is not focused', function () {
      assert.deepEqual(shieldsPanelState.updateActiveTab(state, 2, 4), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 2,
          2: 4
        }
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
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          2: 3
        }
      })
    })
    it('can remove a window which is not focused', function () {
      assert.deepEqual(shieldsPanelState.removeWindowInfo(state, 2), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 2
        }
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
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block'
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
  })
  describe('updateTabShieldsData', () => {
    it('updates the correct tabId data', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateTabShieldsData(state, this.tabId, {
        ads: 'allow',
        trackers: 'allow',
        httpUpgradableResources: 'allow',
        javascript: 'allow',
        fingerprinting: 'allow'
      }), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'allow',
            trackers: 'allow',
            httpUpgradableResources: 'allow',
            javascript: 'allow',
            fingerprinting: 'allow',
            controlsOpen: true,
            braveShields: 'allow',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
  })
  describe('resetBlockingStats', () => {
    it('sets the specified stats back to 0 for the active tab', function () {
      this.tabId = 2
      const stateWithStats: State = {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            adsBlocked: 3,
            trackersBlocked: 4444,
            httpsRedirected: 5,
            javascriptBlocked: 5,
            fingerprintingBlocked: 5,
            controlsOpen: true,
            hostname: 'https://brave.com',
            origin: 'https://brave.com',
            braveShields: 'block',
            url: 'https://brave.com'
          },
          3: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 3,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          },
          4: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 4,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      }

      stateWithStats.tabs[this.tabId].adsBlocked = 3
      stateWithStats.tabs[this.tabId].trackersBlocked = 4
      stateWithStats.tabs[this.tabId].httpsRedirected = 5
      stateWithStats.tabs[this.tabId].javascriptBlocked = 6
      stateWithStats.tabs[this.tabId].fingerprintingBlocked = 7
      assert.deepEqual(shieldsPanelState.resetBlockingStats(stateWithStats, this.tabId), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            origin: 'https://brave.com',
            braveShields: 'block',
            url: 'https://brave.com'
          },
          3: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 3,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          },
          4: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 4,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
    it('sets the specified stats back to 0 for the a non active tab', function () {
      this.tabId = 4
      const stateWithStats: State = {
        currentWindowId: 1,
        tabs: {
          2: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 2,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          },
          3: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 3,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          },
          4: {
            ads: 'block',
            adsBlocked: 3,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 5,
            id: 4,
            javascript: 'block',
            javascriptBlocked: 5,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 4444,
            fingerprinting: 'block',
            fingerprintingBlocked: 5,
            url: 'https://brave.com'
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      }

      assert.deepEqual(shieldsPanelState.resetBlockingStats(stateWithStats, this.tabId), {
        currentWindowId: 1,
        tabs: {
          2: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 2,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          },
          3: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 3,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          },
          4: {
            ads: 'block',
            adsBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            httpUpgradableResources: 'block',
            httpsRedirected: 0,
            id: 4,
            javascript: 'block',
            javascriptBlocked: 0,
            origin: 'https://brave.com',
            braveShields: 'block',
            trackers: 'block',
            trackersBlocked: 0,
            fingerprinting: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com'
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
  })
  describe('updateResourceBlocked', () => {
    it('can update ads blocked count', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'ads'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
    it('can update tracking protection blocked count', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'ads'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0
          },
          3: {
            id: 3
          },
          4: {
            id: 4
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
  })
})
