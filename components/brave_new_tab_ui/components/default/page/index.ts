/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'brave-ui/theme'

const breakpointLargeBlocks = '980px'
const breakpointEveryBlock = '870px'

const singleColumnSmallViewport = css`
 @media screen and (max-width: ${breakpointEveryBlock}) {
   text-align: center;
 }
`

export const Page = styled<{}, 'div'>('div')`
  /* Increase the explicit row count when adding new widgets
     so that the footer goes in the correct location always,
     yet can still merge upwards to previous rows. */
  --ntp-page-rows: 4;
  --ntp-item-justify: start;
  @media screen and (max-width: ${breakpointLargeBlocks}) {
    --ntp-page-rows: 5;
  }
  @media screen and (max-width: ${breakpointEveryBlock}) {
    --ntp-item-justify: center;
  }

  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  position: relative;
  z-index: 3;
  top: 0;
  left: 0;
  display: grid;
  grid-template-rows: repeat(calc(var(--ntp-page-rows) - 1), min-content) auto;
  grid-template-columns: min-content auto min-content;
  grid-auto-flow: row dense;
  padding: 12px;
  overflow: hidden;
  flex: 1;
  flex-direction: column;
  justify-content: space-between;
  height: 100%;
  min-height: 100vh;
  align-items: flex-start;

  @media screen and (max-width: ${breakpointEveryBlock}) {
    display: flex;
    flex-direction: column;
    align-items: center;
  }
`

export const GridItemStats = styled('section')`
  grid-column: 1 / span 2;
  ${singleColumnSmallViewport}
`

export const GridItemClock = styled('section')`
  grid-column: 3;
  justify-self: center;
  ${singleColumnSmallViewport}
`

export const GridItemRewards = styled('section')`
  grid-column: 3 / span 1;
  grid-row-end: span 2;
  @media screen and (max-width: ${breakpointLargeBlocks}) {
    grid-column: 2 / span 2;
    justify-self: end;
  }
`

export const GridItemTopSites = styled('section')`
  grid-column: 1 / span 2;
  ${singleColumnSmallViewport}
`

export const GridItemNotification = styled('section')`
  position: fixed;
  left: 50%;
  top: 0;
  transform: translateX(-50%);
`

export const GridItemCredits = styled('section')`
  grid-column: 1 / span 1;
  grid-row: -3;
  grid-row-end: span 2;
  align-self: end;
  margin: 0 0 36px 36px;
  @media screen and (max-width: ${breakpointEveryBlock}) {
    align-self: center;
    margin: 0;
  }
`

export const GridItemNavigation = styled('section')`
  grid-column: 3 / span 1;
  grid-row-start: -2;
  align-self: end;
  margin: 0 24px 24px 0;
  @media screen and (max-width: ${breakpointEveryBlock}) {
    margin: 0;
    align-self: unset;
  }
`

export const Footer = styled<{}, 'footer'>('footer')`
  display: contents;

  @media screen and (max-width: ${breakpointEveryBlock}) {
    width: 100%;
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    align-items: flex-end;
    flex-wrap: wrap;
  }
`

interface ImageLoadProps {
  imageHasLoaded: boolean
}

interface HasImageProps {
  hasImage: boolean
}

interface AppProps {
  dataIsReady: boolean
}

export const App = styled<AppProps, 'div'>('div')`
  box-sizing: border-box;
  display: flex;
  flex: 1;
  transition: opacity .125s ease-out;
  opacity: ${p => p.dataIsReady ? 1 : 0};
`

export const PosterBackground = styled<ImageLoadProps & HasImageProps, 'div'>('div')`
  position: fixed;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  z-index: 1;
  ${p => !p.hasImage && `
  background: linear-gradient(
        to bottom right,
        #4D54D1,
        #A51C7B 50%,
        #EE4A37 100%);`};
  img {
    width: 100%;
    height: 100%;
    object-fit: cover;
    display: block;
    opacity: ${p => p.imageHasLoaded ? 1 : 0};
    transition: opacity .5s ease-in-out;
  }
`

export const Gradient = styled<ImageLoadProps, 'div'>('div')`
  --gradient-bg: linear-gradient(
    rgba(0, 0, 0, 0.8),
    rgba(0, 0, 0, 0) 35%,
    rgba(0, 0, 0, 0) 80%,
    rgba(0, 0, 0, 0.6) 100%
  );
  z-index: 2;
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  background: var(--gradient-bg);
  /* In dark mode, we don't need an overlay
      unless the image has loaded.
    This prevents a flash of slightly-darker gradient */
  @media (prefers-color-scheme: dark) {
    transition: opacity .4s ease-in-out;
    opacity: ${p => p.imageHasLoaded ? 1 : 0};
  }
  height: 100vh;
`

export const Link = styled<{}, 'a'>('a')`
  text-decoration: none;
  transition: color 0.15s ease, filter 0.15s ease;
  color: rgba(255, 255, 255, 0.8);

  &:hover {
    color: rgba(255, 255, 255, 1);
  }
`

export const PhotoName = styled<{}, 'div'>('div')`
  align-self: flex-end;
  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  font-size: 12px;
  font-family: Muli, sans-serif;
  color: rgba(255, 255, 255, 0.6);
  white-space: nowrap;
`

export const Navigation = styled<{}, 'nav'>('nav')`
  align-self: flex-end;
  display: flex;
  justify-content: flex-end;
`

interface IconButtonProps {
  clickDisabled?: boolean
}

export const IconLink = styled<{}, 'a'>('a')`
  display: flex;
  width: 24px;
  height: 24px;
  margin: 12px;
  cursor: pointer;
  color: #ffffff;
  opacity: 0.7;
  transition: opacity 0.15s ease, filter 0.15s ease;

  &:hover {
    opacity: 0.95;
  }
`

export const IconButton = styled<IconButtonProps, 'button'>('button')`
  pointer-events: ${p => p.clickDisabled && 'none'};
  display: flex;
  width: 24px;
  height: 24px;
  padding: 0;
  border: none;
  margin: 12px;
  cursor: pointer;
  color: #ffffff;
  background-color: transparent;
  opacity: 0.7;
  transition: opacity 0.15s ease, filter 0.15s ease;
  &:hover {
    opacity: 0.95;
  }
`
