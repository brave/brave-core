/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// DND utils
import { Draggable } from 'react-beautiful-dnd'

// Feature-specific components
import { Tile, TileActionsContainer, TileAction } from '../../../../../src/features/newTab/default'

// Icons
import { PinIcon, BookmarkOIcon, TrashIcon } from '../../../../../src/components/icons'
// import { BookmarkIcon } from '../../../../../src/components/icons'

interface Props {
  item: {
    id: string
    content: any
  }
  index: number
}

export default class TopSite extends React.PureComponent<Props, {}> {
  render () {
    const { item, index } = this.props
    return (
      <Draggable key={item.id} draggableId={item.id} index={index}>
      {
        (provided, snapshot) => (
          <Tile
            innerRef={provided.innerRef}
            {...provided.draggableProps}
            {...provided.dragHandleProps}
            isDragging={snapshot.isDragging}
            style={provided.draggableProps.style}
          >
            <TileActionsContainer>
              <TileAction><PinIcon /></TileAction>
              <TileAction><BookmarkOIcon /></TileAction>
              {/* Hover is BookmarkIcon */}
              <TileAction><TrashIcon /></TileAction>
            </TileActionsContainer>
            {item.content}
          </Tile>
        )
      }
      </Draggable>
    )
  }
}
