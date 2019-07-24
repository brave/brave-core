/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { State, PersistentData } from '../../../brave_extension/extension/brave_extension/types/state/shieldsPannelState'
import * as deepFreeze from 'deep-freeze-node'
import * as shieldsPanelState from '../../../brave_extension/extension/brave_extension/state/shieldsPanelState'
import * as noScriptState from '../../../brave_extension/extension/brave_extension/state/noScriptState'
import * as shieldsAPI from '../../../brave_extension/extension/brave_extension/background/api/shieldsAPI'

const state: State = deepFreeze({
  currentWindowId: 1,
  persistentData: {
    isFirstAccess: true
  },
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
    it('Obtains the active tab ID based on the current window', () => {
      expect(shieldsPanelState.getActiveTabId(state)).toEqual(2)
    })
  })
  describe('getActiveTabData', () => {
    it('', () => {
      expect(shieldsPanelState.getActiveTabData(state)).toEqual({
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
  describe('isShieldsActive', () => {
    it('returns false if tab id can not be found', () => {
      const assertion = shieldsPanelState.isShieldsActive(state, 123123123123)
      expect(assertion).toBe(false)
    })
    it('returns false if braveShields is set to `block`', () => {
      const newState: State = deepFreeze({
        ...state,
        tabs: {
          ...state.tabs,
          2: {
            ...state.tabs[2],
            braveShields: 'block'
          }
        }
      })
      const assertion = shieldsPanelState.isShieldsActive(newState, 2)
      expect(assertion).toBe(false)
    })
    it('returns true if braveShields is not set to block', () => {
      const newState: State = deepFreeze({
        ...state,
        tabs: {
          ...state.tabs,
          2: {
            ...state.tabs[2],
            braveShields: 'allow'
          }
        }
      })
      const assertion = shieldsPanelState.isShieldsActive(newState, 2)
      expect(assertion).toBe(true)
    })
  })
  describe('getPersistentData', () => {
    it('is able get persistent data', () => {
      const assertion = shieldsPanelState.getPersistentData(state)
      expect(assertion).toEqual(state.persistentData)
    })
  })
  describe('updatePersistentData', () => {
    it('is able to update persistent data', () => {
      const newPersistentData: Partial<PersistentData> = {
        isFirstAccess: false
      }
      const assertion = shieldsPanelState.updatePersistentData(state, newPersistentData)
      expect(assertion).toEqual({
        ...state,
        persistentData: { ...state.persistentData, ...newPersistentData }
      })
    })
  })
  describe('updateActiveTab', () => {
    it('can update focused window', () => {
      expect(shieldsPanelState.updateActiveTab(state, 1, 4)).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('can update a window which is not focused', () => {
      expect(shieldsPanelState.updateActiveTab(state, 2, 4)).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('can remove the focused window', () => {
      expect(shieldsPanelState.removeWindowInfo(state, 1)).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('can remove a window which is not focused', () => {
      expect(shieldsPanelState.removeWindowInfo(state, 2)).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('updates the currentWindowId', () => {
      expect(shieldsPanelState.updateFocusedWindow(state, 2)).toEqual({
        currentWindowId: 2,
        persistentData: {
          isFirstAccess: true
        },
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
    it('updates the correct tabId data', () => {
      this.tabId = 2
      expect(shieldsPanelState.updateTabShieldsData(state, this.tabId, {
        ads: 'allow',
        trackers: 'allow',
        httpUpgradableResources: 'allow',
        javascript: 'allow',
        fingerprinting: 'allow',
        cookies: 'allow'
      })).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('sets the specified stats back to 0 for the active tab', () => {
      this.tabId = 2
      const stateWithStats: State = {
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
      expect(shieldsPanelState.resetBlockingStats(stateWithStats, this.tabId)).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
            fingerprintingBlockedResources: []
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      })
    })
    it('sets the specified stats back to 0 for the a non active tab', () => {
      this.tabId = 4
      const stateWithStats: State = {
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
            fingerprintingBlockedResources: []
          }
        },
        windows: {
          1: 2,
          2: 3
        }
      }

      expect(shieldsPanelState.resetBlockingStats(stateWithStats, this.tabId)).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('can update ads blocked count', () => {
      this.tabId = 2
      expect(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'ads', 'https://test.brave.com')).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('can update tracking protection blocked count', () => {
      this.tabId = 2
      expect(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'trackers', 'https://test.brave.com')).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
    it('can update javascript blocked count and noScriptInfo', () => {
      this.tabId = 2
      expect(shieldsPanelState.updateResourceBlocked(state, this.tabId, 'javascript', 'https://test.brave.com')).toEqual({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
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
              'https://test.brave.com': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
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
      persistentData: {
        isFirstAccess: true
      },
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
            'https://a.com': { actuallyBlocked: true, willBlock: true, userInteracted: false },
            'https://b.com': { actuallyBlocked: true, willBlock: false, userInteracted: false }
          },
          adsBlockedResources: [],
          trackersBlockedResources: [],
          httpsRedirectedResources: [],
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
            'https://a.com': { actuallyBlocked: true, willBlock: true, userInteracted: false },
            'https://b.com': { actuallyBlocked: false, willBlock: false, userInteracted: false }
          },
          adsBlockedResources: [],
          trackersBlockedResources: [],
          httpsRedirectedResources: [],
          fingerprintingBlockedResources: []
        }
      },
      windows: {
        1: 2,
        2: 3
      }
    }
    it('reset noScriptInfo for a specific tab without navigating away', () => {
      this.tabId = 2
      expect(noScriptState.resetNoScriptInfo(
        stateWithAllowedScriptOrigins, this.tabId, 'https://brave.com')).toEqual({
          currentWindowId: 1,
          persistentData: {
            isFirstAccess: true
          },
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
                'https://a.com': {
                  actuallyBlocked: true,
                  userInteracted: false,
                  willBlock: true
                },
                'https://b.com': {
                  actuallyBlocked: true,
                  userInteracted: false,
                  willBlock: false
                }
              },
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],

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
                'https://a.com': { actuallyBlocked: true, willBlock: true, userInteracted: false },
                'https://b.com': { actuallyBlocked: false, willBlock: false, userInteracted: false }
              },
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],

              fingerprintingBlockedResources: []
            }
          },
          windows: {
            1: 2,
            2: 3
          }
        })
    })
    it('reset noScriptInfo for a specific tab with navigating away', () => {
      this.tabId = 2
      expect(noScriptState.resetNoScriptInfo(
        stateWithAllowedScriptOrigins, this.tabId, 'https://test.brave.com')).toEqual({
          currentWindowId: 1,
          persistentData: {
            isFirstAccess: true
          },
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
                'https://a.com': { actuallyBlocked: true, willBlock: true, userInteracted: false },
                'https://b.com': { actuallyBlocked: false, willBlock: false, userInteracted: false }
              },
              adsBlockedResources: [],
              trackersBlockedResources: [],
              httpsRedirectedResources: [],

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
  describe('updateShieldsIconBadgeText', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.browserAction, 'setBadgeText')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls setBadgeText if tab exists', () => {
      shieldsPanelState.updateShieldsIconBadgeText(state)
      expect(spy).toHaveBeenCalled()
      expect(spy.mock.calls[0][0]).toEqual({ tabId: 2, text: '' })
    })
    it('does not call setBadgeText if tab does not exist', () => {
      const newState: State = deepFreeze({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
        tabs: {},
        windows: { 1: 2 }
      })
      shieldsPanelState.updateShieldsIconBadgeText(newState)
      expect(spy).not.toHaveBeenCalled()
    })
  })
  describe('updateShieldsIconImage', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.browserAction, 'setIcon')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls setIcon if tab exists', () => {
      shieldsPanelState.updateShieldsIconImage(state)
      expect(spy).toHaveBeenCalled()
      expect(spy.mock.calls[0][0]).toEqual({
        tabId: 2,
        path: {
          '18': 'assets/img/shields-on.png',
          '36': 'assets/img/shields-on@2x.png',
          '54': 'assets/img/shields-on@3x.png'
        }
      })
    })
    it('does not call setIcon if tab does not exist', () => {
      const newState: State = deepFreeze({
        currentWindowId: 1,
        persistentData: {
          isFirstAccess: true
        },
        tabs: {},
        windows: { 1: 2 }
      })
      shieldsPanelState.updateShieldsIconImage(newState)
      expect(spy).not.toHaveBeenCalled()
    })
  })
  describe('updateShieldsIcon', () => {
    let updateShieldsIconBadgeTextSpy: jest.SpyInstance
    let updateShieldsIconImageSpy: jest.SpyInstance
    beforeEach(() => {
      updateShieldsIconBadgeTextSpy = jest.spyOn(chrome.browserAction, 'setIcon')
      updateShieldsIconImageSpy = jest.spyOn(chrome.browserAction, 'setIcon')
    })
    afterEach(() => {
      updateShieldsIconBadgeTextSpy.mockRestore()
      updateShieldsIconImageSpy.mockRestore()
    })
    it('calls updateShieldsIconBadgeText', () => {
      shieldsPanelState.updateShieldsIcon(state)
      expect(updateShieldsIconBadgeTextSpy).toHaveBeenCalled()
      expect(updateShieldsIconBadgeTextSpy.mock.calls[0][0]).toEqual({
        path: {
          '18': 'assets/img/shields-on.png',
          '36': 'assets/img/shields-on@2x.png',
          '54': 'assets/img/shields-on@3x.png'
        },
        tabId: 2
      })
    })
    it('calls updateShieldsIconImage', () => {
      shieldsPanelState.updateShieldsIconImage(state)
      expect(updateShieldsIconImageSpy).toHaveBeenCalled()
      expect(updateShieldsIconImageSpy.mock.calls[0][0]).toEqual({
        path: {
          '18': 'assets/img/shields-on.png',
          '36': 'assets/img/shields-on@2x.png',
          '54': 'assets/img/shields-on@3x.png'
        },
        tabId: 2
      })
    })
  })
  describe('focusedWindowChanged', () => {
    let requestShieldPanelDataSpy: jest.SpyInstance
    let updateShieldsIconSpy: jest.SpyInstance
    let consoleWarnSpy: jest.SpyInstance
    beforeEach(() => {
      requestShieldPanelDataSpy = jest.spyOn(shieldsAPI, 'requestShieldPanelData')
      updateShieldsIconSpy = jest.spyOn(shieldsPanelState, 'updateShieldsIcon')
      consoleWarnSpy = jest.spyOn(global.console, 'warn')
    })
    afterEach(() => {
      requestShieldPanelDataSpy.mockRestore()
      updateShieldsIconSpy.mockRestore()
      consoleWarnSpy.mockRestore()
    })
    it('does not modify state if windowId equals -1', () => {
      const assertion = shieldsPanelState.focusedWindowChanged(state, 1)
      expect(assertion).toEqual(state)
    })
    it('updates focused window', () => {
      const focusedWindow = 1337
      const assertion = shieldsPanelState.focusedWindowChanged(state, focusedWindow)
      expect(assertion.currentWindowId).toBe(focusedWindow)
    })
    it('calls requestShieldPanelData if active tab id is defined', () => {
      shieldsPanelState.focusedWindowChanged(state, 1)
      expect(requestShieldPanelDataSpy).toHaveBeenCalled()
      expect(requestShieldPanelDataSpy.mock.calls[0][0]).toEqual(2)
    })
    it('calls updateShieldsIcon if active tab id is defined', () => {
      shieldsPanelState.focusedWindowChanged(state, 1)
      expect(updateShieldsIconSpy).toHaveBeenCalled()
      expect(updateShieldsIconSpy.mock.calls[0][0]).toEqual(state)
    })
    it('calls a console warning when active tab id is not defined', () => {
      shieldsPanelState.focusedWindowChanged(state, 123123123123123123)
      expect(consoleWarnSpy).toBeCalled()
    })
  })
  describe('requestDataAndUpdateActiveTab', () => {
    let requestShieldPanelDataSpy: jest.SpyInstance
    beforeEach(() => {
      requestShieldPanelDataSpy = jest.spyOn(shieldsPanelState, 'updateActiveTab')
    })
    afterEach(() => {
      requestShieldPanelDataSpy.mockRestore()
    })
    it('calls requestShieldPanelData', () => {
      shieldsPanelState.requestDataAndUpdateActiveTab(state, state.windows['1'], state.tabs['2'])
      expect(requestShieldPanelDataSpy).toHaveBeenCalled()
      expect(requestShieldPanelDataSpy.mock.calls[0][0]).toEqual(state)
    })
    it('updates active tab', () => {
      const newState = {
        ...state,
        tabs: {
          ...state.tabs,
          2: {
            ...state.tabs[2],
            NEW_UPDATED_PROPERTY: {}
          }
        }
      }
      const assertion = shieldsPanelState
        .requestDataAndUpdateActiveTab(newState, 1, 2)
      expect(assertion).toEqual({ ...state, ...newState })
    })
  })
})
