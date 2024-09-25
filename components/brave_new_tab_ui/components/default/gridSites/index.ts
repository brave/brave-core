// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled, { css } from 'styled-components'

export const TileTitle = styled('p')<{}>`
  margin: 0;
  font: ${leo.font.small.semibold};
  text-shadow: ${leo.effect.elevation['02']};
  max-width: 100%;
  height: 17px;
  color: var(--override-readability-color, white);
  padding: 0 2px;
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
`

export const TileImageContainer = styled('div')<{}>`
  position: relative;
  width: 56px;
  height: 56px;
  border-radius: 16px;
  background: #FFFFFF40;
  backdrop-filter: blur(8px);
  display: flex;
  align-items: center;
  justify-content: center;

  &::before {
    content: "";
    position: absolute;
    inset: 0;
    border-radius: 16px;
    padding: 1px;
    background: linear-gradient(
      156.52deg,
      rgba(0, 0, 0, 0.05) 2.12%,
      rgba(0, 0, 0, 0) 39%,
      rgba(0, 0, 0, 0) 54.33%,
      rgba(0, 0, 0, 0.15) 93.02%);
    mask:
      linear-gradient(#fff 0 0) content-box,
      linear-gradient(#fff 0 0);
    mask-composite: exclude;
  }

  &:hover {
    background: rgba(255, 255, 255, .35);
  }
`

export interface AddSiteTileProps {
  isDragging: boolean
}

export const AddSiteTile = styled('button')<AddSiteTileProps>`
  background: transparent;
  width: 72px;
  height: 107px;
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

  --leo-icon-size: 32px;
  --leo-icon-color: rgba(255, 255, 255, .7);

  ${p => p.isDragging && css`
    visibility: hidden;
  `}

  &:focus-visible, &:focus {
    ${TileImageContainer} {
      outline: 4px solid rgba(255, 255, 255, 0.6);
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
  --grid-column-width: 72px;

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
  width: 20px;
  height: 20px;
  z-index: 1;
  top: -4px;
  right: 2px;
  display: flex;
`

export const TileMenu = styled('div')<{}>`
  position: absolute;

  margin-left: 4px;
  margin-top: -4px;

  min-width: 150px;
  height: 72px;
  padding: 8px 0;
  display: flex;
  flex-direction: column;
  border-radius: 4px;
  box-shadow: 0px 0px 16px 0px rgba(0, 0, 0, 0.3);
  z-index: 10;

  --leo-icon-size: 14px;

  background: white;
  @media (prefers-color-scheme: dark) {
    background: #3B3E4F;
  }
`

export const TileMenuItem = styled('button')<{}>`
  width: 100%;
  height: 30px;
  font: ${leo.font.small.regular};
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

export const TileAction = styled('button').attrs({ 'data-theme': 'light' })`
  -webkit-appearance: none;
  box-sizing: border-box;
  transition: color 0.1s linear;
  background: #fff;
  width: 100%;
  border-radius: 50%;
  margin: 0;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  outline: unset;
  background-clip: padding-box;
  border: none;

  --leo-icon-size: 14px;
  --leo-icon-color: ${leo.color.icon.interactive};

  &:focus-visible {
    outline: 4px rgba(255, 255, 255, 0.6);
  }

  &:active {
    background-clip: padding-box;
  }
`

export const TileFavicon = styled('img')<{}>`
  width: 32px;
  height: 32px;
  object-fit: contain;
  border-radius: 8px;
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
  width: 72px;
  height: 107px;
  cursor: pointer;
  // Menu goes behind in other Tiles when tils has z-index.
  // Give z-index while dragging to make dragging tile moves over other tiles.
  z-index: ${p => p.isDragging ? 3 : 'unset'}
  outline: unset;
  gap: 8px;

  ${p => !p.isMenuShowing && css`
    &:active ${TileImageContainer} {
      outline: 4px solid rgba(255, 255, 255, 0.6);
    }
  `}

  &:focus-visible {
    outline: none;

    ${TileImageContainer} {
      outline: 4px solid rgba(255, 255, 255, 0.6);
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
