// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Reducer } from '@reduxjs/toolkit'
import {
  PersistedState,
  PersistConfig,
  persistReducer
} from 'redux-persist'

import storage from 'redux-persist/lib/storage'
import autoMergeLevel2 from 'redux-persist/lib/stateReconciler/autoMergeLevel2'

/**
 * Increment this version number when:
 * - removing queries form the api slice
 * - removing state from the redux store
 */
export const PERSISTED_STATE_VERSION = 1

/**
 * Uses the enhanced config object to persist a reducer
 * with a type-safe whitelist
 *
 * Clears the persisted state when the version changes
 * to prevent storing old/unused data
 *
 * @param reducer The reducer to persist
 * @param config options for how to persist the state
 * @returns persisted reducer
 */
export const persistVersionedReducer = <
  R extends Reducer,
  C extends Omit<PersistConfig<ReturnType<R>>, 'storage' | 'blacklist'> & {
    whitelist?: Array<keyof ReturnType<R>>
  }
>(
  reducer: R,
  config: C
): Reducer<PersistedState & ReturnType<R>> => {
  const revertToInitialStateOnVersionChange = async <
    T extends PersistedState & ReturnType<R>
  >(
    state: T
  ): Promise<T> => {
    // remove persisted API state when persisted state versions change
    if (
      state?._persist.version &&
      state?._persist.version !== (config.version || PERSISTED_STATE_VERSION)
    ) {
      return {
        ...state,
        [config.key]: reducer(undefined, { type: '' })
      }
    }

    return state
  }

  const persistConfig: PersistConfig<ReturnType<R>> = {
    storage,
    migrate: revertToInitialStateOnVersionChange,
    stateReconciler: autoMergeLevel2,
    ...config
  }

  return persistReducer(persistConfig, reducer)
}
