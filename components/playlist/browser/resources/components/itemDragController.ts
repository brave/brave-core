// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  DragController,
  InitialState,
  Target
} from '../utils/dragDropController'

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

type TargetItem = Target & {
  item: PlaylistItem
  height: number
}

type ItemInitialState = InitialState & {
  offsetTop: number
}

export class ItemDragController extends DragController<
  ItemInitialState,
  TargetItem
> {
  constructor () {
    super('vertical')

    this.initialState = {
      clientPos: 0,
      target: undefined,
      offsetTop: 0
    }
  }

  onClickItem: (item: PlaylistItem) => void
  onDragItemStarted: (item: PlaylistItem) => void
  onDragItemUpdated: (item: PlaylistItem, indexDelta: number) => void
  onDragItemEnded: (item: PlaylistItem) => void

  onDragStart = (target: TargetItem, clientPos: number) => {
    target.elementRef.current!.style.zIndex = '5'
    this.initialState.offsetTop = target.elementRef.current!.offsetTop
    this.onDragItemStarted(target.item)
  }

  onDragUpdate = (target: TargetItem, clientPos: number) => {
    const offset = clientPos - this.initialState.clientPos
    let indexDelta = Math.floor(offset / target.height)
    if (indexDelta < 0) indexDelta++

    this.onDragItemUpdated(target.item, indexDelta)

    target.elementRef.current!.style.transform = `translateY(${
      offset +
      this.initialState.offsetTop -
      target.elementRef.current!.offsetTop
    }px`
  }

  onDragEnd = (target: TargetItem, clientPos: number) => {
    target.elementRef.current!.style.zIndex = ''
    target.elementRef.current!.style.transform = ''

    this.onDragItemEnded(target.item)
  }

  onClick = (target: TargetItem) => {
    this.onClickItem(target.item)
  }
}
