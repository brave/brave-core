/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  DragSource,
  DragSourceCollector,
  DragSourceConnector,
  DragSourceMonitor,
  DragSourceSpec,
  DropTarget,
  DropTargetCollector,
  DropTargetConnector,
  DropTargetMonitor,
  DropTargetSpec
} from 'react-dnd'

// Feature-specific components
import { Tile, TileActionsContainer, TileAction, TileFavicon } from '../../components/default'

// Icons
import { PinIcon, PinOIcon, BookmarkOIcon, BookmarkIcon, CloseStrokeIcon } from 'brave-ui/components/icons'

const Types = {
  BLOCK: 'block'
}

const blockSource: DragSourceSpec<Props> = {
  /**
   * Required. Called when the dragging starts
   * It's the only data available to the drop targets about the drag source
   * @see http://gaearon.github.io/react-dnd/docs-drag-source.html#specification-methods
   */
  beginDrag (props: Props) {
    return {
      id: props.id
    }
  },

  endDrag (props: Props, monitor: DragSourceMonitor) {
    const item: Props = monitor.getItem() as Props
    const draggedId = item.id
    const didDrop = monitor.didDrop()
    props.onDragEnd(draggedId, didDrop)
  }
}

const blockTarget: DropTargetSpec<Props> = {
  /**
   * Optional. Called when an item is hovered over the component
   * @see http://gaearon.github.io/react-dnd/docs-drop-target.html#specification-methods
   */
  hover (props: Props, monitor: DropTargetMonitor) {
    const item: Props = monitor.getItem() as Props
    const draggedId = item.id
    if (draggedId !== props.id) {
      const dragRight =
        monitor.getClientOffset().x - monitor.getInitialSourceClientOffset().x >
        0
      props.onDraggedSite(draggedId, props.id, dragRight)
    }
  }
}

/**
 * Both sourceCollect and targetCollect are called *Collecting Functions*
 * They will be called by React DnD with a connector that lets you connect
 * nodes to the DnD backend, and a monitor to query information about the drag state.
 * It should return a plain object of props to inject into your component.
 *
 * @see http://gaearon.github.io/react-dnd/docs-drop-target.html#the-collecting-function
 */

const sourceCollect: DragSourceCollector = (
  connect: DragSourceConnector,
  monitor: DragSourceMonitor
) => {
  return {
    connectDragSource: connect.dragSource(),
    isDragging: monitor.isDragging()
  }
}

const targetCollect: DropTargetCollector = (connect: DropTargetConnector) => {
  return {
    connectDropTarget: connect.dropTarget()
  }
}

interface Props {
  id: string
  onDragEnd: (draggedId: string, didDrop: boolean) => void
  onDraggedSite: (draggedId: string, id: string, dragRight: boolean) => void
  connectDragSource?: any
  connectDropTarget?: any
  onToggleBookmark: () => void
  isBookmarked?: boolean
  onPinnedTopSite: () => void
  isPinned: boolean
  onIgnoredTopSite: () => void
  title: string
  href: string
  style: {
    backgroundColor: string
  }
  favicon: string
}

// TODO remove so many props NZ
class Block extends React.Component<Props, {}> {
  render () {
    const {
      connectDragSource,
      connectDropTarget,
      onToggleBookmark,
      isBookmarked,
      onPinnedTopSite,
      isPinned,
      onIgnoredTopSite,
      title,
      href,
      style,
      favicon
    } = this.props
    const starIcon = isBookmarked ? <BookmarkIcon /> : <BookmarkOIcon />
    const pinIcon = isPinned ? <PinIcon /> : <PinOIcon />

    return connectDragSource(
      connectDropTarget(
        <div>
          <Tile title={title} style={style}>
            <TileActionsContainer>
              <TileAction onClick={onPinnedTopSite}>
                {pinIcon}
              </TileAction>
              <TileAction onClick={onToggleBookmark}>
                {starIcon}
              </TileAction>
              <TileAction onClick={onIgnoredTopSite}>
                <CloseStrokeIcon />
              </TileAction>
            </TileActionsContainer>
            {
              isPinned
              ? <TileAction onClick={onPinnedTopSite} standalone={true}><PinIcon /></TileAction>
              : null
            }
            <a href={href}>
              <TileFavicon src={favicon} />
            </a>
          </Tile>
        </div>
      )
    )
  }
}

/**
 * Wraps the component to make it draggable
 * Only the drop targets registered for the same type will
 * react to the items produced by this drag source.
 *
 * @see http://gaearon.github.io/react-dnd/docs-drag-source.html
 */
const source = DragSource<Props>(Types.BLOCK, blockSource, sourceCollect)(
  Block
)

// Notice that we're exporting the DropTarget and not Block Class.
/**
 * React to the compatible items being dragged, hovered, or dropped on it
 * Works with the same parameters as DragSource() above.
 *
 * @see http://gaearon.github.io/react-dnd/docs-drop-target.html
 */
export default DropTarget<Props>(Types.BLOCK, blockTarget, targetCollect)(
  source
)
