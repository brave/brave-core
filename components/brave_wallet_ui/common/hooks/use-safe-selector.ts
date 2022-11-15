// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { useSelector } from 'react-redux'

import { WalletPageState, WalletPanelState } from '../../constants/types'

type WalletStoreState = Omit<WalletPageState, 'page'>
type PageStoreState = Omit<WalletPageState, 'wallet'>
type PanelStoreState = Omit<WalletPanelState, 'wallet'>

type PrimitiveType = string | boolean | number | null

type SubsetSelector<TState, TBanned, TSelected> = (state: TState) => TSelected extends TBanned ? never : TSelected

type SafeSelector<TState, TSelected> = (state: TState) => TSelected extends PrimitiveType | undefined ? TSelected : never

type TypedUseSelectorHookWithBannedSelections<TState, TBanned> = <TSelected>(
  selector: SubsetSelector<TState, TBanned, TSelected>,
  equalityFn?: (left: TSelected, right: TSelected) => boolean
) => TSelected

type TypedUseSafeSelectorHook<TState> = <TSelected>(
  selector: SafeSelector<TState, TSelected>,
  equalityFn?: (left: TSelected, right: TSelected) => boolean
) => TSelected

export const useUnsafeWalletSelector: TypedUseSelectorHookWithBannedSelections<
  WalletStoreState,
  PrimitiveType
> = useSelector

export const useUnsafePanelSelector: TypedUseSelectorHookWithBannedSelections<
  PanelStoreState,
  PrimitiveType
> = useSelector

export const useUnsafePageSelector: TypedUseSelectorHookWithBannedSelections<
  PageStoreState,
  PrimitiveType
> = useSelector

export const useSafeWalletSelector: TypedUseSafeSelectorHook<WalletStoreState> = useSelector
export const useSafePanelSelector: TypedUseSafeSelectorHook<PanelStoreState> = useSelector
export const useSafePageSelector: TypedUseSafeSelectorHook<PageStoreState> = useSelector
