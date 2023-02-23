// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled, { css } from 'styled-components'

export const TileTitle = styled('p')<{}>`
  margin: 0;
  font-family: Poppins;
  font-weight: 400;
  font-size: 11px;
  line-height: 17px;
  max-width: 100%;
  height: 17px;
  color: var(--override-readability-color, white);
  padding: 0 2px;
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
`

export const AddSiteTileImage = styled('div')<{}>`
  background: rgba(255, 255, 255, 0.2);
  backdrop-filter: blur(8px);
  border-radius: 8px;
  width: 70px;
  height: 70px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;

  &:hover {
    background: rgba(255, 255, 255, 0.4);
  }
`

export interface AddSiteTileProps {
  isDragging: boolean
}

export const AddSiteTile = styled('button')<AddSiteTileProps>`
  background: transparent;
  width: 78px;
  height: 110px;
  cursor: pointer;
  border: none;
  margin: 0;
  padding: 0;
  outline: unset;
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: center;
  gap: 8px;

  ${p => p.isDragging && css`
    visibility: hidden;
  `}

  &:focus-visible, :focus {
    gap: 4px;

    ${AddSiteTileImage} {
      margin-top: -4px;
      width: 78px;
      height: 78px;
      background-clip: padding-box;
      border: 4px solid white;
    }
  }
`

export const List = styled('div')`
  height: 100%;
  justify-self: start;

  // Add right padding of one column, so there's a nice gap between pages.
  padding-right: var(--grid-column-width);
  
  display: grid;
  justify-content: var(--ntp-item-justify, start);
  grid-template-columns: repeat(var(--grid-columns), var(--grid-column-width));

  scroll-snap-align: start;
`

export const PagesContainer = styled('div')`
  max-width: 100vw;

  display: flex;
  flex-direction: column;
  align-items: center;

  margin-bottom: 24px;

  --grid-columns: 6;
  --grid-column-width: 86px;

  @media screen and (max-width: 700px) {
    --grid-columns: 4;
  }

  @media screen and (max-width: 360px) {
    --grid-columns: 3;
  }
`

export const GridPagesContainer = styled('div')<{ customLinksEnabled: boolean }>`
  display: flex;
  flex-direction: row;

  margin-left: 24px;
  padding: 24px 24px 0 24px;
  max-width: calc((var(--grid-columns) + 1) * var(--grid-column-width));
  overflow-x: ${p => p.customLinksEnabled ? 'auto' : 'hidden'};

  scroll-snap-type: x mandatory;
  scroll-snap-stop: always;

  @media screen and (max-width: 870px) {
    margin-left: var(--grid-column-width);
  }

  ::-webkit-scrollbar {
    display: none;
  }
`

export const TileActionsContainer = styled('nav')<{}>`
  box-sizing: border-box;
  opacity: 0;
  visibility: hidden;
  transition: 0.15s opacity linear;
  position: absolute;
  width: 40px;
  height: 40px;
  z-index: 1;
  top: -12px;
  right: -12px;
  display: flex;
`

export const TileMenu = styled('div')<{}>`
  position: absolute;

  // Here we apply a negative 12px offset so the menu has a 12px overlap with
  // the button.
  margin-left: -12px;
  margin-top: -12px;

  min-width: 185px;
  height: 72px;
  padding: 8px 0;
  display: flex;
  flex-direction: column;
  border-radius: 4px;
  box-shadow: 0px 0px 16px 0px rgba(0, 0, 0, 0.3);
  z-index: 10;

  background: white;
  @media (prefers-color-scheme: dark) {
    background: #3B3E4F;
  }
`

export const TileMenuItem = styled('button')<{}>`
  width: 100%;
  height: 30px;
  font-family: Poppins;
  font-size: 13px;
  font-style: normal;
  font-weight: 500;
  line-height: 20px;
  letter-spacing: 0.01em;
  text-align: left;
  margin: 0;
  border: none;
  outline: unset;
  padding: 4px 13px;
  background: inherit;
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 11px;
  cursor: pointer;
  color: ${p => p.theme.color.contextMenuHoverForeground};

  &:hover, :focus-visible {
    background-color: ${p => p.theme.color.contextMenuHoverBackground};
    color: ${p => p.theme.color.contextMenuHoverForeground};
  }
`

export const TileAction = styled('button')<{}>`
  -webkit-appearance: none;
  box-sizing: border-box;
  transition: color 0.1s linear;
  background: #DADCEB;
  width: 100%;
  border-radius: 50%;
  margin: 0;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  outline: unset;
  background-clip: padding-box;
  border: 4px solid transparent;

  &:focus-visible {
    border: 4px solid rgba(255, 255, 255, 0.6);
  }

  &:active {
    background: #AEB1C2;
    border: 4px solid transparent;
    background-clip: padding-box;
  }
`

export const TileFavicon = styled('img')<{}>`
  display: block;
  padding: 16px;
  width: 70px;
  height: 70px;
  box-shadow: 0px 1px 4px 0px rgba(0, 0, 0, 0.3);
  border-radius: 8px;
  object-fit: contain;

  background: ${p => p.theme.palette.white};
  @media (prefers-color-scheme: dark) {
    background: ${p => p.theme.palette.black};
  }
`

interface TileProps {
  isDragging: boolean
  isMenuShowing: boolean
}

export const Tile = styled('a')<TileProps>`
  position: relative;
  text-decoration: none;
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: center;
  width: 78px;
  height: 110px;
  cursor: pointer;
  // Menu goes behind in other Tiles when tils has z-index.
  // Give z-index while dragging to make dragging tile moves over other tiles.
  z-index: ${p => p.isDragging ? 3 : 'unset'}
  outline: unset;
  gap: 8px;

  ${p => !p.isMenuShowing && css`
    &:active {
      gap: 4px;

      ${TileFavicon} {
        margin-top: -4px;
        width: 78px;
        height: 78px;
        background-clip: padding-box;
        border: 4px solid rgba(255, 255, 255, 0.6);
      }
    }
  `}

  &:focus-visible {
    gap: 4px;
    outline: none;

    ${TileFavicon} {
      margin-top: -4px;
      width: 78px;
      height: 78px;
      background-clip: padding-box;
      border: 4px solid rgba(255, 255, 255, 0.6);
    }
  }

  ${p => !p.isDragging && !p.isMenuShowing && css`
    &:hover {
      ${TileActionsContainer} {
        opacity: 1;
        visibility: visible;
      }
    }
  `}

  ${p => p.isMenuShowing && css`
    ${TileActionsContainer} {
      opacity: 1;
      visibility: visible;
    }
  `}
`
