// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createSelector,
  EntityId,
  EntityState,
  Selector
} from '@reduxjs/toolkit'

export type EntityByIdFromRegistryQuerySelector<T> = Selector<
  any,
  T | undefined,
  [EntityId]
>
export type EntityByIdFromRegistryResultSelectorFactory<T> =
  () => EntityByIdFromRegistryQuerySelector<T>

export function makeSelectEntityByIdFromRegistryQuery<T> () {
  return createSelector(
    // get data from query response
    (res: { data?: EntityState<T> | undefined }) => res.data,
    // get id arg
    (res: any, id: EntityId) => id,
    // select the entity by id
    (data: EntityState<T> | undefined, id: string) => data?.entities[id]
  )
}
