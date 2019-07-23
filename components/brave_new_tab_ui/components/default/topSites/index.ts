/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'
import createWidget from '../widget'

export const List = styled<{}, 'div'>('div')`
  padding: 0 70px;
  height: 100%;
  display: grid;
  grid-template-columns: repeat(6, 92px);

  @media screen and (max-width: 1150px) {
    justify-content: center;
    padding: 40px;
  }

  @media screen and (max-width: 630px) {
    grid-template-columns: repeat(3, 92px);
  }

  @media screen and (max-width: 390px) {
    grid-template-columns: repeat(2, 92px);
  }
`

export const TileActionsContainer = styled<{}, 'nav'>('nav')`
  box-sizing: border-box;
  opacity: 0;
  visibility: hidden;
  transition: 0.15s opacity linear;
  position: absolute;
  z-index: 2;
  top: 0;
  left: 0;
  right: 0;
  text-align: center;
  display: flex;
  justify-content: space-between;
  margin: 6px;
  border-radius: 4px;
  background-color: #FFFFFF;
`

interface TileActionProps {
  standalone?: boolean
}

export const TileAction = styled<TileActionProps, 'a'>('a')`
  box-sizing: border-box;
  transition: color 0.1s linear;
  color: #424242;
  font-size: 14px;
  width: 20px;
  height: 20px;
  padding: 2px 4px;
  background: ${p => p.standalone && '#FFFFFF'};
  position: ${p => p.standalone && 'absolute'};
  top: ${p => p.standalone && '6px'};
  left: ${p => p.standalone && '6px'};
  border-radius: ${p => p.standalone && '4px'};
  margin: 0;
  text-decoration: none;
  display: block;
  cursor: pointer;

  &:hover {
    color: #000;
  }
`

interface TileProps {
  isDragging?: boolean
}

export const Tile = styled<TileProps, 'div'>('div')`
  background-color: ${p => p.isDragging ? 'rgba(255, 255, 255, 0.5)' : 'rgba(255, 255, 255, 0.8)'};
  position: relative;
  user-select: none;
  margin: 6px;
  display: flex;
  justify-content: center;
  align-items: center;
  box-shadow: 1px 1px 6px 2px rgba(0,0,0,0.3);
  border-radius: 8px;
  width: 80px;
  height: 80px;
  font-size: 38px;

  &:hover {
    ${TileActionsContainer} {
      opacity: 1;
      visibility: visible;
    }
  }
`

export const TileFavicon = styled<{}, 'img'>('img')`
  display: block;
  height: 72px;
  padding: 16px;
`

export const ListWidget = createWidget(List)
