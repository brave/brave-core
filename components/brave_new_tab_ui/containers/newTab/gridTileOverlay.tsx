// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { DragOverlay, useDndContext } from '@dnd-kit/core'
import * as React from 'react'
import { SiteTile } from './gridTile'

export function TopSiteDragOverlay (props: { sites: NewTab.Site[] }) {
    const { active } = useDndContext()
    const dragging = active && props.sites.find(s => s.id === active.id)
    return <DragOverlay>
      {dragging && <SiteTile site={dragging}/>}
    </DragOverlay>
  }
