/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// DND utils
import { Draggable } from 'react-beautiful-dnd'

// Feature-specific components
import { Tile, TileActionsContainer, TileAction } from '../../../../../src/features/newTab/default'

// Icons
import { AlertShieldIcon, SendIcon, CloseStrokeIcon } from '../../../../../src/components/icons'

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
              <TileAction><AlertShieldIcon /></TileAction>
              <TileAction><SendIcon /></TileAction>
              <TileAction><CloseStrokeIcon /></TileAction>
            </TileActionsContainer>
            {item.content}
          </Tile>
        )
      }
      </Draggable>
    )
  }
}
