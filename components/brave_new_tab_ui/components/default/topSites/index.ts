/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'

export const List = styled<{}, 'div'>('div')`
  padding: 0;
  height: 100%;
  display: flex;
  justify-content: flex-end;
`

interface TileProps {
  isDragging: boolean
}

export const Tile = styled<TileProps, 'div'>('div')`
  box-sizing: border-box;
  background-color: ${p => p.isDragging ? 'lightgray' : '#fff'};
  position: relative;
  user-select: none;
  margin: 0 12px 0 0;
  display: flex;
  justify-content: center;
  align-items: center;
  box-shadow: 1px 1px 6px 2px rgba(0,0,0,0.3);
  border-radius: 8px;
  width: 80px;
  height: 80px;
  font-size: 38px;
`

export const TileActionsContainer = styled<{}, 'nav'>('nav')`
  box-sizing: border-box;
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  text-align: center;
  display: flex;
  justify-content: space-between;
  padding: 6px 8px;
`
// these needs hover, and active states for the icons. BookmarkIcon and PinIcon

export const TileAction = styled<{}, 'a'>('a')`
  box-sizing: border-box;
  color: #000;
  width: 12px;
  height: 12px;
  font-size: 12px;
  padding: 0;
  margin: 0;
  text-decoration: none;
  display: block;
`
