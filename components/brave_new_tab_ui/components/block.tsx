/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { cx } from '../../common/classSet'
import {
  DragSource, DragSourceCollector,
  DragSourceConnector,
  DragSourceMonitor, DragSourceSpec,
  DropTarget, DropTargetCollector,
  DropTargetConnector,
  DropTargetMonitor, DropTargetSpec
} from 'react-dnd'

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
      const dragRight = monitor.getClientOffset().x - monitor.getInitialSourceClientOffset().x > 0
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

const sourceCollect: DragSourceCollector = (connect: DragSourceConnector, monitor: DragSourceMonitor) => {
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
export default class Block extends React.Component<Props, {}> {
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
    const starIcon = isBookmarked ? 'fa-star' : 'fa-star-o'
    const pinIcon = isPinned ? 'fa-minus' : 'fa-thumb-tack'

    return connectDragSource(connectDropTarget(
      <div className='topSiteSquareSpace'>
        <div
          className='topSitesElement'
        >
          <div className='topSitesActionContainer'>
            <button
              className={cx({
                topSitesActionBtn: true,
                fa: true,
                [pinIcon]: true
              })}
              onClick={onPinnedTopSite}
              data-l10n-id={isPinned ? 'pinTopSiteButton' : 'unpinTopSiteButton'}
            />
            <button
              className={cx({
                topSitesActionBtn: true,
                fa: true,
                [starIcon]: true
              })}
              onClick={onToggleBookmark}
              data-l10n-id={isBookmarked ? 'removeBookmarkButton' : 'addBookmarkButton'}
            />
            <button
              className='topSitesActionBtn fa fa-close'
              onClick={onIgnoredTopSite}
              data-l10n-id='removeTopSiteButton'
            />
          </div>
          <a
            className='topSitesElementFavicon'
            title={title}
            href={href}
            style={style}
          >
            {isPinned ? <div className='pinnedTopSite'><span className='pin fa fa-thumb-tack' /></div> : null}
            <img src={favicon} />
          </a>
        </div>
      </div>
    ))
  }
}

/**
 * Wraps the component to make it draggable
 * Only the drop targets registered for the same type will
 * react to the items produced by this drag source.
 *
 * @see http://gaearon.github.io/react-dnd/docs-drag-source.html
 */
const source = DragSource<Props>(Types.BLOCK, blockSource, sourceCollect)(Block)

// Notice that we're exporting the DropTarget and not Block Class.
/**
 * React to the compatible items being dragged, hovered, or dropped on it
 * Works with the same parameters as DragSource() above.
 *
 * @see http://gaearon.github.io/react-dnd/docs-drop-target.html
 */
export const block = DropTarget<Props>(Types.BLOCK, blockTarget, targetCollect)(source)
