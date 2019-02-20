/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as deepFreeze from 'deep-freeze-node'
import * as shieldsPanelState from '../../state/shieldsPanelState'
import { State } from '../../types/state/shieldsPannelState'

const state: State = deepFreeze({
  currentWindowId: 1,
  tabs: {
    2: {
      id: 2,
      ads: 'block',
      trackers: 'block',
      httpUpgradableResources: 'block',
      javascript: 'block',
      fingerprinting: 'block',
      cookies: 'block'
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
        fingerprinting: 'block',
        cookies: 'block'
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
            fingerprinting: 'block',
            cookies: 'block'
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
            fingerprinting: 'block',
            cookies: 'block'
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
            fingerprinting: 'block',
            cookies: 'block'
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
            fingerprinting: 'block',
            cookies: 'block'
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
            fingerprinting: 'block',
            cookies: 'block'
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
        fingerprinting: 'allow',
        cookies: 'allow'
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
            cookies: 'allow',
            controlsOpen: true,
            braveShields: 'allow',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            adsBlocked: 3,
            trackersBlocked: 4444,
            httpsRedirected: 5,
            javascriptBlocked: 5,
            fingerprintingBlocked: 5,
            controlsOpen: true,
            hostname: 'https://brave.com',
            origin: 'https://brave.com',
            braveShields: 'block',
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            hostname: 'https://brave.com',
            origin: 'https://brave.com',
            braveShields: 'block',
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 5,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
            cookies: 'block',
            fingerprintingBlocked: 0,
            url: 'https://brave.com',
            noScriptInfo: {},
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
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
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'ads', 'https://test.brave.com'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            noScriptInfo: {},
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            trackersBlockedResources: []
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
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'trackers', 'https://test.brave.com'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            adsBlocked: 0,
            trackersBlocked: 1,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            noScriptInfo: {},
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            trackersBlockedResources: [ 'https://test.brave.com' ]
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
    it('can update javascript blocked count and noScriptInfo', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'javascript', 'https://test.brave.com'), {
        currentWindowId: 1,
        tabs: {
          2: {
            id: 2,
            ads: 'block',
            trackers: 'block',
            httpUpgradableResources: 'block',
            javascript: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 1,
            fingerprintingBlocked: 0,
            noScriptInfo: {
              'https://test.brave.com/': { actuallyBlocked: true, willBlock: true }
            },
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [ 'https://test.brave.com' ],
            trackersBlockedResources: []
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
  describe('resetNoScriptInfo', () => {
    const stateWithAllowedScriptOrigins: State = {
      currentWindowId: 1,
      tabs: {
        2: {
          id: 2,
          ads: 'block',
          adsBlocked: 0,
          controlsOpen: true,
          hostname: 'https://brave.com',
          httpUpgradableResources: 'block',
          httpsRedirected: 0,
          javascript: 'block',
          javascriptBlocked: 0,
          origin: 'https://brave.com',
          braveShields: 'block',
          trackers: 'block',
          trackersBlocked: 0,
          fingerprinting: 'block',
          cookies: 'block',
          fingerprintingBlocked: 0,
          url: 'https://brave.com',
          noScriptInfo: {
            'https://a.com': { actuallyBlocked: true, willBlock: true },
            'https://b.com': { actuallyBlocked: true, willBlock: false }
          },
          adsBlockedResources: [],
          trackersBlockedResources: [],
          httpsRedirectedResources: [],
          javascriptBlockedResources: [],
          fingerprintingBlockedResources: []
        },
        3: {
          id: 3,
          ads: 'block',
          adsBlocked: 0,
          controlsOpen: true,
          hostname: 'https://brave.com',
          httpUpgradableResources: 'block',
          httpsRedirected: 0,
          javascript: 'block',
          javascriptBlocked: 0,
          origin: 'https://brave.com',
          braveShields: 'block',
          trackers: 'block',
          trackersBlocked: 0,
          fingerprinting: 'block',
          cookies: 'block',
          fingerprintingBlocked: 0,
          url: 'https://brave.com',
          noScriptInfo: {
            'https://a.com': { actuallyBlocked: true, willBlock: true },
            'https://b.com': { actuallyBlocked: false, willBlock: false }
          },
          adsBlockedResources: [],
          trackersBlockedResources: [],
          httpsRedirectedResources: [],
          javascriptBlockedResources: [],
          fingerprintingBlockedResources: []
        }
      },
      windows: {
        1: 2,
        2: 3
      }
    }
    it('reset noScriptInfo for a specific tab without navigating away', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.resetNoScriptInfo(
        stateWithAllowedScriptOrigins, this.tabId, 'https://brave.com'), {
          currentWindowId: 1,
          tabs: {
            2: {
              id: 2,
              ads: 'block',
              adsBlocked: 0,
              controlsOpen: true,
              hostname: 'https://brave.com',
              httpUpgradableResources: 'block',
              httpsRedirected: 0,
              javascript: 'block',
              javascriptBlocked: 0,
              origin: 'https://brave.com',
              braveShields: 'block',
              trackers: 'block',
              trackersBlocked: 0,
              fingerprinting: 'block',
              cookies: 'block',
              fingerprintingBlocked: 0,
              url: 'https://brave.com',
              noScriptInfo: {
                'https://b.com': { actuallyBlocked: false, willBlock: false }
              },
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],
              javascriptBlockedResources: [],
              fingerprintingBlockedResources: []
            },
            3: {
              id: 3,
              ads: 'block',
              adsBlocked: 0,
              controlsOpen: true,
              hostname: 'https://brave.com',
              httpUpgradableResources: 'block',
              httpsRedirected: 0,
              javascript: 'block',
              javascriptBlocked: 0,
              origin: 'https://brave.com',
              braveShields: 'block',
              trackers: 'block',
              trackersBlocked: 0,
              fingerprinting: 'block',
              cookies: 'block',
              fingerprintingBlocked: 0,
              url: 'https://brave.com',
              noScriptInfo: {
                'https://a.com': { actuallyBlocked: true, willBlock: true },
                'https://b.com': { actuallyBlocked: false, willBlock: false }
              },
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],
              javascriptBlockedResources: [],
              fingerprintingBlockedResources: []
            }
          },
          windows: {
            1: 2,
            2: 3
          }
      })
    })
    it('reset noScriptInfo for a specific tab with navigating away', function () {
      this.tabId = 2
      assert.deepEqual(shieldsPanelState.resetNoScriptInfo(
        stateWithAllowedScriptOrigins, this.tabId, 'https://test.brave.com'), {
          currentWindowId: 1,
          tabs: {
            2: {
              id: 2,
              ads: 'block',
              adsBlocked: 0,
              controlsOpen: true,
              hostname: 'https://brave.com',
              httpUpgradableResources: 'block',
              httpsRedirected: 0,
              javascript: 'block',
              javascriptBlocked: 0,
              origin: 'https://brave.com',
              braveShields: 'block',
              trackers: 'block',
              trackersBlocked: 0,
              fingerprinting: 'block',
              cookies: 'block',
              fingerprintingBlocked: 0,
              url: 'https://brave.com',
              noScriptInfo: {},
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],
              javascriptBlockedResources: [],
              fingerprintingBlockedResources: []
            },
            3: {
              id: 3,
              ads: 'block',
              adsBlocked: 0,
              controlsOpen: true,
              hostname: 'https://brave.com',
              httpUpgradableResources: 'block',
              httpsRedirected: 0,
              javascript: 'block',
              javascriptBlocked: 0,
              origin: 'https://brave.com',
              braveShields: 'block',
              trackers: 'block',
              trackersBlocked: 0,
              fingerprinting: 'block',
              cookies: 'block',
              fingerprintingBlocked: 0,
              url: 'https://brave.com',
              noScriptInfo: {
                'https://a.com': { actuallyBlocked: true, willBlock: true },
                'https://b.com': { actuallyBlocked: false, willBlock: false }
              },
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],
              javascriptBlockedResources: [],
              fingerprintingBlockedResources: []
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
