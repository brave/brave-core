/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../../../brave_rewards/resources/extension/brave_rewards/background/reducers'
import { types } from '../../../../../../brave_rewards/resources/extension/brave_rewards/constants/rewards_panel_types'
import { defaultState } from '../../../../../../brave_rewards/resources/extension/brave_rewards/background/storage'

describe('rewards panel reducer', () => {
  const constantDate = new Date('2018-01-01T12:00:00')

  beforeAll(() => {
    (global as any).Date = class extends Date {
      constructor () {
        super()
        return constantDate
      }
    }
  })

  describe('ON_TAB_RETRIEVED', () => {
    describe('persist publisher info', () => {
      it('url is the same', () => {
        const initState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true
        }
        const payload = {
          tab: {
            url: 'https://clifton.io',
            incognito: false,
            active: true,
            id: 1
          }
        }

        // first visit
        const expectedState1: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabId: 1,
              tabUrl: 'https://clifton.io'
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload
        })

        expect(state.rewardsPanelData).toEqual(expectedState1)

        // imitates ON_PUBLISHER_DATA
        state.rewardsPanelData.publishers = {
          key_1: {
            tabUrl: 'https://clifton.io',
            publisherKey: 'clifton.io',
            name: 'Clifton'
          }
        }

        // second visit
        const expectedState2: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabId: 1,
              tabUrl: 'https://clifton.io',
              publisherKey: 'clifton.io',
              name: 'Clifton'
            }
          }
        }

        state = reducers(state, {
          type: types.ON_TAB_RETRIEVED,
          payload
        })

        expect(state.rewardsPanelData).toEqual(expectedState2)
      })

      it('url is the same, but publisher was not saved correctly', () => {
        const initState: Rewards.State = { ...defaultState, walletCreated: true }
        const payload = {
          tab: {
            url: 'https://clifton.io',
            incognito: false,
            active: true,
            id: 1
          }
        }

        // first visit
        const expectedState1: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          publishers: {
            key_1: {
              tabId: 1,
              tabUrl: 'https://clifton.io'
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload
        })

        expect(state.rewardsPanelData).toEqual(expectedState1)

        // imitates ON_PUBLISHER_DATA
        state.rewardsPanelData.publishers = {
          key_1: {
            tabUrl: 'https://clifton.io'
          }
        }

        // second visit
        const expectedState2: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          publishers: {
            key_1: {
              tabUrl: 'https://clifton.io'
            }
          }
        }

        state = reducers(state, {
          type: types.ON_TAB_RETRIEVED,
          payload
        })

        expect(state.rewardsPanelData).toEqual(expectedState2)
      })

      it('url is not the same', () => {
        const initState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true
        }

        // first visit
        const expectedState1: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabId: 1,
              tabUrl: 'https://clifton.io'
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://clifton.io',
              incognito: false,
              active: true,
              id: 1
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState1)

        // imitates ON_PUBLISHER_DATA
        state.rewardsPanelData.publishers = {
          key_1: {
            tabUrl: 'clifton.io',
            publisherKey: 'clifton.io',
            name: 'Clifton'
          }
        }

        // second visit
        const expectedState2: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabId: 1,
              tabUrl: 'https://brave.com'
            }
          }
        }

        state = reducers(state, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://brave.com',
              incognito: false,
              active: true,
              id: 1
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState2)
      })

      it('rewards are disabled', () => {
        const initState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: false
        }

        const expectedState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: false
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://clifton.io',
              incognito: false,
              active: true,
              id: 1
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState)
      })

      it('publisher is empty', () => {
        const initState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabUrl: 'clifton.io',
              random: 'I need to vanish'
            }
          }
        }

        const expectedState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabId: 1,
              tabUrl: 'https://clifton.io'
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://clifton.io',
              incognito: false,
              active: true,
              id: 1
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState)
      })

      it('switching between two tabs that have the same url', () => {
        const initState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabUrl: 'https://clifton.io',
              publisherKey: 'clifton.io',
              tabId: 1
            }
          }
        }

        const expectedState: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          enabledMain: true,
          publishers: {
            key_1: {
              tabUrl: 'https://clifton.io',
              publisherKey: 'clifton.io',
              tabId: 1
            },
            key_2: {
              tabUrl: 'https://clifton.io',
              tabId: 2
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://clifton.io',
              incognito: false,
              active: true,
              windowId: 1,
              id: 2
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState)
      })
    })
  })

  describe('ON_PUBLISHER_LIST_NORMALIZED', () => {
    it('list is empty', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_PUBLISHER_LIST_NORMALIZED,
        payload: {
          properties: []
        }
      })

      expect(state.rewardsPanelData).toEqual(defaultState)
    })

    it('list is undefined', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_PUBLISHER_LIST_NORMALIZED,
        payload: {
          properties: undefined
        }
      })

      expect(state.rewardsPanelData).toEqual(defaultState)
    })

    it('publisher is update accordingly', () => {
      const list = [
        {
          publisherKey: 'brave.com',
          percentage: 50,
          status: 2
        },
        {
          publisherKey: 'brave1.com',
          percentage: 30,
          status: 2
        },
        {
          publisherKey: 'brave2.com',
          percentage: 10,
          status: 2
        },
        {
          publisherKey: 'brave3.com',
          percentage: 10,
          status: 2
        }
      ]

      let state = {
        ...defaultState,
        publishers: {
          key_1: {
            tabUrl: 'https://brave.com',
            publisherKey: 'brave.com',
            percentage: 30,
            status: 0,
            excluded: true
          },
          key_2: {
            tabUrl: 'https://brave4.com',
            publisherKey: 'brave4.com',
            percentage: 40,
            status: 2
          }
        }
      }
      const expectedState: Rewards.State = {
        ...defaultState,
        publishers: {
          key_1: {
            tabUrl: 'https://brave.com',
            publisherKey: 'brave.com',
            percentage: 50,
            status: 2,
            excluded: false
          },
          key_2: {
            tabUrl: 'https://brave4.com',
            publisherKey: 'brave4.com',
            percentage: 0,
            status: 2
          }
        }
      }

      state = reducers({ rewardsPanelData: state }, {
        type: types.ON_PUBLISHER_LIST_NORMALIZED,
        payload: {
          properties: list
        }
      })

      expect(state.rewardsPanelData).toEqual(expectedState)
    })
  })

  describe('ON_EXCLUDED_SITES_CHANGED', () => {
    it('properties is undefined', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_EXCLUDED_SITES_CHANGED,
        payload: {
          properties: undefined
        }
      })

      expect(state.rewardsPanelData).toEqual(defaultState)
    })

    it('publisher key is undefined', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_EXCLUDED_SITES_CHANGED,
        payload: {
          properties: {
            publisherKey: ''
          }
        }
      })

      expect(state.rewardsPanelData).toEqual(defaultState)
    })

    it('publisher is update accordingly', () => {
      let state = {
        ...defaultState,
        publishers: {
          key_1: {
            tabUrl: 'https://brave.com',
            publisherKey: 'brave.com',
            percentage: 30,
            status: 2,
            excluded: true
          },
          key_2: {
            tabUrl: 'https://brave4.com',
            publisherKey: 'brave4.com',
            percentage: 40,
            status: 2
          }
        }
      }
      const expectedState: Rewards.State = {
        ...defaultState,
        publishers: {
          key_1: {
            tabUrl: 'https://brave.com',
            publisherKey: 'brave.com',
            percentage: 30,
            status: 2,
            excluded: false
          },
          key_2: {
            tabUrl: 'https://brave4.com',
            publisherKey: 'brave4.com',
            percentage: 40,
            status: 2
          }
        }
      }

      state = reducers({ rewardsPanelData: state }, {
        type: types.ON_EXCLUDED_SITES_CHANGED,
        payload: {
          properties: {
            publisherKey: 'brave.com',
            excluded: false
          }
        }
      })

      expect(state.rewardsPanelData).toEqual(expectedState)
    })
  })

  describe('ON_ALL_NOTIFICATIONS', () => {
    it('list is undefined', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_ALL_NOTIFICATIONS,
        payload: {
          list: undefined
        }
      })

      expect(state.rewardsPanelData).toEqual(defaultState)
    })

    it('list is empty and it was not defined before', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_ALL_NOTIFICATIONS,
        payload: {
          list: []
        }
      })

      const expectedState: Rewards.State = {
        ...defaultState,
        notifications: {}
      }

      expect(state.rewardsPanelData).toEqual(expectedState)
    })

    it('list is ok', () => {
      let state = reducers({ rewardsPanelData: defaultState }, {
        type: types.ON_ALL_NOTIFICATIONS,
        payload: {
          list: [
            {
              id: 'notification_1',
              type: 1,
              timestamp: 0,
              args: ['working']
            },
            {
              id: 'notification_2',
              type: 1,
              timestamp: 1,
              args: []
            }
          ]
        }
      })

      const expectedState: Rewards.State = {
        ...defaultState,
        currentNotification: 'notification_2',
        notifications: {
          'notification_1': {
            id: 'notification_1',
            type: 1,
            timestamp: 0,
            args: ['working']
          },
          'notification_2': {
            id: 'notification_2',
            type: 1,
            timestamp: 1,
            args: []
          }
        }
      }

      expect(state.rewardsPanelData).toEqual(expectedState)
    })
  })
})
