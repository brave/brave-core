/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// DND utils
import { Draggable } from 'react-beautiful-dnd'

// Feature-specific components
import { Tile, TileActionsContainer, TileAction, TileFavicon } from '../../../components/default'

// Icons
import { PinIcon, BookmarkOIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

interface Props {
  item: {
    id: string
    name: string
    url: string
    favicon: string
    background: string
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
            style={Object.assign({}, provided.draggableProps.style, { background: item.background })}
          >
            <TileActionsContainer>
              <TileAction><PinIcon /></TileAction>
              <TileAction><BookmarkOIcon /></TileAction>
              {/* Hover is BookmarkIcon */}
              <TileAction><CloseStrokeIcon /></TileAction>
            </TileActionsContainer>
              <TileFavicon src={item.favicon} />
          </Tile>
        )
      }
      </Draggable>
    )
  }
}
