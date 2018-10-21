/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// DND utils
import { DragDropContext, Droppable } from 'react-beautiful-dnd'
import { reorder, getItems } from '../helpers'

// Feature-specific components
import { List } from '../../../../../src/features/newTab/default'

// Component group
import TopSite from './topSite'

interface Props {}

interface State {
  items: Array<any>
}

export default class TopSitesList extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { items: getItems(6) }
  }

  onDragEnd = (result: any) => {
    // dropped outside the list
    if (!result.destination) {
      return
    }

    const items = reorder(this.state.items, result.source.index, result.destination.index)
    this.setState({ items })
  }

  render () {
    return (
      <DragDropContext onDragEnd={this.onDragEnd}>
        <Droppable droppableId='droppable' direction='horizontal'>
          {(provided) => {
            return (
              <List {...provided.droppableProps} innerRef={provided.innerRef}>
                {this.state.items.map((item, index) => <TopSite item={item} index={index} key={index} />)}
                {provided.placeholder}
              </List>
            )
          }}
        </Droppable>
      </DragDropContext>
    )
  }
}
