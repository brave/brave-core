/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultState } from './default_state'
import { createLocalStorageScope } from '../../shared/lib/local_storage_scope'
import { userTypeFromString } from '../../shared/lib/user_type'

const storageScope = createLocalStorageScope<'state'>('rewards')

function readCachedState (state: Rewards.State) {
  // This is tricky. We assert that the stored state is a valid instance of
  // the page state even though it might not be, so that TypeScript can check
  // the property accesses for us. However, the state may have been stored by an
  // earlier version of the browser or may otherwise be corrupted. It's OK for
  // this function to throw if a property access fails, but any data pulled out
  // of the cached object must be coerced to the correct type:
  //
  // - Atomic values must be coerced using value constructors.
  // - Subobjects must be built out and not directly assigned.
  // - Arrays must be mapped to valid entries.
  //
  // Not every state value needs to be read from cache. Only read values that
  // are persistent or necessary for first-render.

  const cached = storageScope.readJSON('state') as Rewards.State
  if (!cached) {
    return state
  }

  return {
    ...state,
    userType: userTypeFromString(String(cached.userType || '')),
    enabledAds: Boolean(cached.enabledAds),
    enabledAdsMigrated: Boolean(cached.enabledAdsMigrated),
    enabledContribute: Boolean(cached.enabledContribute),
    contributionMinTime: Number(cached.contributionMinTime) || 0,
    contributionMinVisits: Number(cached.contributionMinVisits) || 0,
    contributionMonthly: Number(cached.contributionMonthly) || 0,
    contributionNonVerified: Boolean(cached.contributionNonVerified),
    contributionVideos: Boolean(cached.contributionVideos),
    reconcileStamp: Number(cached.reconcileStamp),
    ui: {
      ...state.ui,
      promosDismissed: Object.fromEntries(
        Object.entries(cached.ui.promosDismissed).map(([key, value]) => {
          return [key, Boolean(value)]
        }))
    },
    adsData: {
      ...state.adsData,
      adsEnabled: Boolean(cached.adsData.adsEnabled),
      adsPerHour: Number(cached.adsData.adsPerHour) || 0,
      adsSubdivisionTargeting:
        String(cached.adsData.adsSubdivisionTargeting || ''),
      automaticallyDetectedAdsSubdivisionTargeting:
        String(cached.adsData.automaticallyDetectedAdsSubdivisionTargeting ||
               ''),
      shouldAllowAdsSubdivisionTargeting:
        Boolean(cached.adsData.shouldAllowAdsSubdivisionTargeting)
    },
    balance: {
      ...state.balance,
      total: Number(cached.balance.total) || 0
    },
    currentCountryCode: String(cached.currentCountryCode),
    parameters: {
      ...state.parameters,
      autoContributeChoice: Number(cached.parameters.autoContributeChoice) || 0,
      rate: Number(cached.parameters.rate) || 0
    }
  }
}

export function loadState (): Rewards.State {
  let state = defaultState()

  try {
    state = readCachedState(state)
  } catch (e) {
    console.error('Error reading cached state', e)
  }

  return state
}

export function saveState (state: Rewards.State) {
  storageScope.writeJSON('state', state)
}
