// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useDispatch, useSelector, TypedUseSelectorHook } from 'react-redux'

// TODO: When react-redux is upgraded to 8.1.0+, migrate to the simpler
// withTypes pattern:
//   export const useAppDispatch = useDispatch.withTypes<AppDispatch>()
//   export const useAppSelector = useSelector.withTypes<RootState>()

import type {
  RootStoreState as PageRootStoreState,
  WalletPageRootStore,
} from '../../page/store'
import type { RootStoreState as PanelRootStoreState } from '../../panel/store'

// Union type that works for both page and panel contexts.
// At runtime, only one store exists per context.
export type RootState = PageRootStoreState | PanelRootStoreState

// Dispatch type from the page store. Both stores use identical middleware
// configuration, so this type works for both contexts.
export type AppDispatch = WalletPageRootStore['dispatch']

/**
 * Pre-typed useDispatch hook for the wallet stores.
 * Use this instead of plain useDispatch for full type safety.
 *
 * @see https://redux.js.org/usage/migrating-to-modern-redux
 */
export const useAppDispatch: () => AppDispatch = useDispatch

/**
 * Pre-typed useSelector hook for the wallet stores.
 * Use this instead of plain useSelector for full type safety.
 *
 * Note: For selectors that return objects/arrays, prefer using the
 * memoized selectors from common/selectors or useSafe*Selector hooks
 * to avoid unnecessary re-renders.
 *
 * @see https://redux.js.org/usage/migrating-to-modern-redux
 */
export const useAppSelector: TypedUseSelectorHook<RootState> = useSelector
