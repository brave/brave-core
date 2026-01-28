/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { AppStore, defaultAppStore } from './app_store'

const sampleLog = `\
--------------------------------------------------------------------------------
[Mar 13, 2025 12:57:33.1 PM GMT:INFO:rewards_service_impl.cc(397)] Starting engine process
[Mar 13, 2025 12:57:34.2 PM GMT:INFO:database_migration.cc(154)] DB: Migrated to version 1
[Mar 13, 2025 12:57:34.2 PM GMT:INFO:database_migration.cc(154)] DB: Migrated to version 2
[Mar 13, 2025 12:57:34.2 PM GMT:INFO:database_migration.cc(154)] DB: Migrated to version 3
[Mar 13, 2025 12:57:34.2 PM GMT:INFO:database_migration.cc(154)] DB: Migrated to version 4
[Mar 13, 2025 12:57:34.2 PM GMT:INFO:database_migration.cc(154)] DB: Migrated to version 5
[Mar 13, 2025 12:57:34.2 PM GMT:INFO:database_migration.cc(154)] DB: Migrated to version 6
`

export function createAppStore(): AppStore {
  const store = defaultAppStore()
  store.update({
    externalWallet: {
      provider: 'uphold',
      authenticated: true,
      name: 'Account name',
      url: 'https://brave.com',
    },
    adDiagnosticEntries: [
      {
        name: 'Device Id',
        value:
          'aee1118bb026ae7571069a7a2243f0e560f5a2b45b28f468495ad4034aad577f',
      },
      {
        name: 'Opted into Brave News ads',
        value: 'true',
      },
      {
        name: 'Catalog ID',
        value: 'KvJb/azOiqUaVq0Mh1oViRmyflvjhg3rvhSFqpdE96I=',
      },
    ],
  })

  store.update({
    actions: {
      getString(key) {
        switch (key) {
          case 'fullLogDisclaimerText':
            return 'WARNING: This log file may contain sensitive data. Be careful who you share it with.'
          case 'pageDisclaimerText':
            return 'WARNING: Data on these pages may be sensitive. Be careful who you share them with.'
          case 'pageTitle':
            return 'Rewards internals'
        }
      },

      setAdDiagnosticId(diagnosticId) {
        store.update({ adDiagnosticId: diagnosticId })
      },

      clearRewardsLog() {
        store.update({
          rewardsLog: '',
        })
      },

      loadRewardsLog() {
        store.update({
          rewardsLog: sampleLog,
        })
      },

      async fetchFullRewardsLog() {
        return 'full log'
      },

      loadContributions() {
        store.update({
          contributions: [
            {
              id: '812e6649-ba2b-4b8e-8925-163e401007c8',
              amount: 0.25,
              type: 'one-time',
              step: 0,
              retryCount: -1,
              createdAt: Date.now(),
              processor: 'uphold',
              publishers: [
                {
                  id: 'github#21312',
                  totalAmount: 1,
                  contributedAmount: 1,
                },
                {
                  id: 'youtube#9989',
                  totalAmount: 1,
                  contributedAmount: 1,
                },
              ],
            },
            {
              id: '812e6649-ba2b-4b8e-8925-163e401007c8',
              amount: 0.25,
              type: 'one-time',
              step: 0,
              retryCount: -1,
              createdAt: Date.now(),
              processor: 'uphold',
              publishers: [
                {
                  id: 'github#21312',
                  totalAmount: 1,
                  contributedAmount: 1,
                },
                {
                  id: 'youtube#9989',
                  totalAmount: 1,
                  contributedAmount: 1,
                },
              ],
            },
          ],
        })
      },

      loadRewardsEvents() {
        store.update({
          rewardsEvents: [
            {
              id: '1',
              key: 'wallet-connected',
              value: 'uphold/1234',
              createdAt: Date.now(),
            },
          ],
        })
      },
    },
  })

  return store
}
