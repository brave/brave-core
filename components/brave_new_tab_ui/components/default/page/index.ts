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

interface PageProps {
  showClock: boolean
  showStats: boolean
  showRewards: boolean
  showBinance: boolean
  showTogether: boolean
  showTopSites: boolean
  showBrandedWallpaper: boolean
}

function getItemRowCount (p: PageProps): number {
  let right = (p.showClock ? 1 : 0) + (p.showRewards ? 2 : 0)
  let left = (p.showStats ? 1 : 0) + (p.showTopSites ? 1 : 0)
  // Has space for branded logo to sit next to something on right?
  if (p.showBrandedWallpaper && left >= right) {
    left++
  }
  return Math.max(left, right) + 1 // extra 1 for footer
}

export const Page = styled<PageProps, 'div'>('div')`
  /* Increase the explicit row count when adding new widgets
     so that the footer goes in the correct location always,
     yet can still merge upwards to previous rows. */
  --ntp-item-row-count: ${getItemRowCount};
  --ntp-extra-footer-rows: ${p => p.showBrandedWallpaper ? 1 : 0};
  --ntp-space-rows: 0;
  --ntp-page-rows: calc(var(--ntp-item-row-count) + var(--ntp-space-rows));
  --ntp-item-justify: start;
  @media screen and (max-width: ${breakpointLargeBlocks}) {
    --ntp-space-rows: 1;
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

export const GridItemWidgetStack = styled('section')`
  grid-column: 3 / span 1;
  grid-row-end: span 2;
  @media screen and (max-width: ${breakpointLargeBlocks}) {
    max-width: 284px;
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
  /* Variables for easy inherited override without splitting css rules definition */
  --ntp-grid-item-credits-bottom-margin-wide: 36px;
  --ntp-grid-item-credits-left-margin-narrow: 10px;
  --ntp-grid-item-credits-left-margin-wide: var(--ntp-grid-item-credits-bottom-margin-wide);
  grid-column: 1 / span 1;
  grid-row: calc(-2 - var(--ntp-extra-footer-rows)) / span calc(1 + var(--ntp-extra-footer-rows));
  align-self: end;

  margin: 0 0 var(--ntp-grid-item-credits-bottom-margin-wide) var(--ntp-grid-item-credits-left-margin-wide);
  @media screen and (max-width: ${breakpointEveryBlock}) {
    /* Display on left, keeping Navigation on right even on wrapped row. */
    margin: 0 auto 0 var(--ntp-grid-item-credits-left-margin-narrow);
    align-self: unset;
  }
`

export const GridItemBrandedLogo = styled(GridItemCredits)`
  --ntp-grid-item-credits-left-margin-narrow: 0;
  --ntp-grid-item-credits-bottom-margin-wide: -8px;
  --ntp-grid-item-credits-left-margin-wide: 22px;
`

export const GridItemNavigation = styled('section')`
  grid-column: 3 / span 1;
  grid-row: -2 / span 1;
  align-self: end;
  margin: 0 24px 24px 0;
  @media screen and (max-width: ${breakpointEveryBlock}) {
    margin: 0;
    align-self: flex-end;
  }
`

export const Footer = styled<{}, 'footer'>('footer')`
  /* Child items are primary Grid items and can slot in to free spaces,
     so this element doesn't do anything on wider viewport widths. */
  display: contents;

  @media screen and (max-width: ${breakpointEveryBlock}) {
    width: 100%;
    /* Take up rest of Page height so that footer is always at bottom */
    flex: 1;
    display: flex;
    flex-direction: row;
    align-items: flex-end;
  }
`

export const FooterContent = styled('div')`
  display: contents;

  @media screen and (max-width: ${breakpointEveryBlock}) {
    width: 100%;
    display: flex;
    flex-direction: row;
    justify-content: flex-end;
    align-items: center;
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
  height: 40px;
  align-items: center;
`

interface IconButtonProps {
  clickDisabled?: boolean
  isClickMenu?: boolean
}

export const IconLink = styled<{}, 'a'>('a')`
  display: block;
  width: 24px;
  height: 24px;
  margin: 8px;
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
  margin: ${p => p.isClickMenu ? '7' : '0 12'}px;
  cursor: pointer;
  color: #ffffff;
  background-color: transparent;
  opacity: 0.7;
  transition: opacity 0.15s ease, filter 0.15s ease;
  &:hover {
    opacity: 0.95;
  }
`

interface IconButtonSideTextProps {
  textDirection: string
}

export const IconButtonSideText = styled<IconButtonSideTextProps, 'label'>('label')`
  display: grid;
  grid-template-columns: auto auto;
  align-items: center;
  margin-right: ${p => p.textDirection === 'ltr' && '24px'};
  margin-left: ${p => p.textDirection === 'rtl' && '24px'};
  color: inherit;
  cursor: pointer;
  user-select: none;

  &:focus-within {
    /* get the browser defaults */
    outline-color: rgba(0, 103, 244, 0.247);
    outline-style: auto;
    outline-width: 5px;
  }

  > ${IconButton} {
    margin-left: ${p => p.textDirection === 'ltr' && '0'};
    margin-right: ${p => p.textDirection === 'rtl' && '0'};
    /* No need to show the outline since the parent is handling it */
    outline: 0;
  }
`
