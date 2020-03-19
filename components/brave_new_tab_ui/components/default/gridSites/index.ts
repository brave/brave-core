// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'brave-ui/theme'

interface ListProps {
  blockNumber: number
}

export const List = styled<ListProps, 'div'>('div')`
  justify-self: start;
  align-items: normal;
  height: 100%;
  display: grid;
  grid-template-columns: repeat(${p => Math.min(p.blockNumber, 6).toString()}, 92px);
  justify-content: var(--ntp-item-justify, start);

  @media screen and (max-width: 700px) {
    grid-template-columns: repeat(${p => Math.min(p.blockNumber, 3).toString()}, 92px);
  }

  @media screen and (max-width: 390px) {
    grid-template-columns: repeat(${p => Math.min(p.blockNumber, 2).toString()}, 92px);
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

export const TileAction = styled<TileActionProps, 'button'>('button')`
  -webkit-appearance: none;
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
  display: block;
  cursor: pointer;
  border: 0;

  &:hover {
    color: #000;
  }
`

interface TileProps {
  isDragging?: boolean
}

export const Tile = styled<TileProps, 'div'>('div')`
  background-color: #ffffff;
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
  z-index: 3;
  cursor: grab;

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
  object-fit: contain;
`

export const ListWidget = List
