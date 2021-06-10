/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'
import { requestAnimationFrameThrottle } from '../../../../common/throttle'

const breakpointLargeBlocks = '980px'
const breakpointEveryBlock = '870px'
const CLASSNAME_PAGE_STUCK = 'page-stuck'

const singleColumnSmallViewport = css`
 @media screen and (max-width: ${breakpointEveryBlock}) {
   text-align: center;
 }
`

interface HasImageProps {
  hasImage: boolean
  imageHasLoaded: boolean
  imageSrc?: string
}

type AppProps = {
  dataIsReady: boolean
} & HasImageProps

type PageProps = {
  showClock: boolean
  showStats: boolean
  showRewards: boolean
  showBinance: boolean
  showTogether: boolean
  showTopSites: boolean
  showBrandedWallpaper: boolean
} & HasImageProps

function getItemRowCount (p: PageProps): number {
  let right = (p.showClock ? 1 : 0) + (p.showRewards ? 2 : 0)
  let left = (p.showStats ? 1 : 0) + (p.showTopSites ? 1 : 0)
  // Has space for branded logo to sit next to something on right?
  if (p.showBrandedWallpaper && left >= right) {
    left++
  }
  return Math.max(left, right) + 1 // extra 1 for footer
}

const StyledPage = styled('div')<PageProps>`
  /* Increase the explicit row count when adding new widgets
     so that the footer goes in the correct location always,
     yet can still merge upwards to previous rows. */
  --ntp-item-row-count: ${getItemRowCount};
  --ntp-extra-footer-rows: ${p => p.showBrandedWallpaper ? 1 : 0};
  --ntp-space-rows: 0;
  --ntp-page-rows: calc(var(--ntp-item-row-count) + var(--ntp-space-rows));
  --ntp-page-padding: 12px;
  --ntp-item-justify: start;
  --blur-amount: calc(var(--ntp-extra-content-effect-multiplier, 0) * 38px);
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
  width: 100%;
  display: grid;
  grid-template-rows: repeat(calc(var(--ntp-page-rows) - 1), min-content) auto;
  grid-template-columns: min-content auto min-content;
  grid-auto-flow: row dense;
  padding: var(--ntp-page-padding);
  overflow: hidden;
  flex: 1;
  flex-direction: column;
  justify-content: space-between;
  min-height: 100vh;
  align-items: flex-start;

  /* Fix the main NTP content so, when Brave Today is in-view,
  NTP items remain in the same place, and still allows NTP
  Page to scroll to the bottom before that starts happening. */
  .${CLASSNAME_PAGE_STUCK} & {
    position: fixed;
    bottom: 0;
    /* Blur out the content when Brave Today is interacted
      with. We need the opacity to fade out our background image.
      We need the background image to overcome the bug
      where a backdrop-filter element's ancestor which has
      a filter must also have a background. When this bug is
      fixed then this element won't need the background.
    */
    opacity: calc(1 - var(--ntp-extra-content-effect-multiplier));
    filter: blur(var(--blur-amount));
    background: var(--default-bg-color);
    ${getPageBackground}
  }

  @media screen and (max-width: ${breakpointEveryBlock}) {
    display: flex;
    flex-direction: column;
    align-items: center;
  }
`

export const Page: React.FunctionComponent<PageProps> = (props) => {
  // Note(petemill): When we scroll to the bottom, if there's an
  // extra scroll area (Brave Today) then we "sticky" the Page at
  // the bottom scroll and overlay the extra content on top.
  // This isn't possible with regular `position: sticky` as far as I can tell.
  const pageRef = React.useRef<HTMLDivElement>(null)
  React.useEffect(() => {
    const element = pageRef.current
    if (!element) {
      console.error('no element')
      return
    }
    const root = document.querySelector<HTMLElement>('#root')

    const sub = requestAnimationFrameThrottle(() => {
      const viewportHeight = window.innerHeight
      const scrollBottom = window.scrollY + viewportHeight
      const scrollPast = scrollBottom - element.clientHeight
      if (scrollPast >= 1) {
        // Have blur effect amount follow scroll amount. Should
        // be fully blurred at 50% of viewport height
        const blurUpperLimit = viewportHeight * .65
        const blurLowerLimit = viewportHeight * .25
        const blurAmount = scrollPast > blurUpperLimit
          ? 1
          : scrollPast < blurLowerLimit
            ? 0
            : (scrollPast - blurLowerLimit) / (blurUpperLimit - blurLowerLimit)
        if (root) {
          root.style.setProperty('--ntp-extra-content-effect-multiplier', blurAmount.toString())
          root.style.setProperty('--ntp-fixed-content-height', Math.round(element.clientHeight) + 'px')
          root.classList.add(CLASSNAME_PAGE_STUCK)
        }
      } else {
        if (root) {
          root.style.setProperty('--ntp-extra-content-effect-multiplier', '0')
          root.style.setProperty('--ntp-fixed-content-height', '0px')
          root.classList.remove(CLASSNAME_PAGE_STUCK)
        }
      }
    })

    window.addEventListener('scroll', sub)
    window.addEventListener('resize', sub)
    sub()
    return () => {
      window.removeEventListener('scroll', sub)
      window.removeEventListener('resize', sub)
    }
  }, [])
  return (
    <StyledPage ref={pageRef} {...props}>
      {props.children}
    </StyledPage>
  )
}

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

  @media screen and (min-width: ${breakpointEveryBlock}) {
    position: fixed;
    bottom: var(--ntp-page-padding);
    left: var(--ntp-page-padding);
    .${CLASSNAME_PAGE_STUCK} & {
      // When page is also position: fixed, then we are relative to that
      bottom: 0;
      left: 0;
    }
  }
`

export const GridItemNavigation = styled('section')`
  grid-column: 2 / span 2;
  grid-row: -2 / span 1;
  align-self: end;
  margin: 0 24px 24px 0;
  @media screen and (max-width: ${breakpointEveryBlock}) {
    margin: 0;
    align-self: flex-end;
  }
`

export const GridItemNavigationBraveToday = styled('div')<{}>`
  position: absolute;
  bottom: 20px;
  left: 50%;
  transform: translate(-50%, 0);
  margin: 0 auto;
`

export const Footer = styled('footer')<{}>`
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

function getPageBackground (p: HasImageProps) {
  // Page background is duplicated since a backdrop-filter's
  // ancestor which has blur must also have background.
  // In our case, Widgets are the backdrop-filter element
  // and Page is the element with blur (when brave today is active)
  // so it also needs the background. However, we also
  // need the background _not_ to blur, so we also put it on
  // Page's ancestor: App.
  // Use a :before pseudo element so that we can fade the image
  // in when it is loaded.
  return css<HasImageProps>`
    &:before {
      pointer-events: none;
      content: "";
      position: fixed;
      top: 0;
      bottom: 0;
      left: 0;
      z-index: -1;
      right: 0;
      display: block;
      transition: opacity .5s ease-in-out;
      ${p => !p.hasImage && css`
        background: linear-gradient(
            to bottom right,
            #4D54D1,
            #A51C7B 50%,
            #EE4A37 100%);
      `};
      ${p => p.hasImage && p.imageSrc && css`
        opacity: var(--bg-opacity);
        background: linear-gradient(
              rgba(0, 0, 0, 0.8),
              rgba(0, 0, 0, 0) 35%,
              rgba(0, 0, 0, 0) 80%,
              rgba(0, 0, 0, 0.6) 100%
            ), url(${p.imageSrc});
        background-size: cover;
        background-repeat: no-repeat;
        background-attachment: fixed;
      `};
      background-position: center center;
    }
  `
}

export const App = styled('div')<AppProps & HasImageProps>`
  --bg-opacity: ${p => p.imageHasLoaded ? 1 : 0};
  position: relative;
  padding-top: var(--ntp-fixed-content-height, "0px");
  box-sizing: border-box;
  display: flex;
  flex: 1;
  flex-direction: column;
  transition: opacity .125s ease-out;
  opacity: ${p => p.dataIsReady ? 1 : 0};
  ${getPageBackground}
`

export const Link = styled('a')<{}>`
  text-decoration: none;
  transition: color 0.15s ease, filter 0.15s ease;
  color: rgba(255, 255, 255, 0.8);

  &:hover {
    color: rgba(255, 255, 255, 1);
  }
`

export const Label = styled('span')<{}>`
  text-decoration: none;
  transition: color 0.15s ease, filter 0.15s ease;
  color: rgba(255, 255, 255, 0.8);

  &:hover {
    color: rgba(255, 255, 255, 1);
  }
`

export const PhotoName = styled('div')<{}>`
  align-self: flex-end;
  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  font-size: 12px;
  font-family: Muli, sans-serif;
  color: rgba(255, 255, 255, 0.6);
  white-space: nowrap;
`

export const Navigation = styled('nav')<{}>`
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

export const IconLink = styled('a')<{}>`
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

export const IconButton = styled('button')<IconButtonProps>`
  pointer-events: ${p => p.clickDisabled && 'none'};
  display: flex;
  width: 24px;
  height: 24px;
  padding: 0;
  border: none;
  outline: none;
  margin: ${p => p.isClickMenu ? '7' : '0 12'}px;
  cursor: pointer;
  color: #ffffff;
  background-color: transparent;
  opacity: 0.7;
  transition: opacity 0.15s ease, filter 0.15s ease;
  &:hover {
    opacity: 0.95;
  }
  &:focus-visible {
    outline: 2px solid ${p => p.theme.color.brandBraveInteracting};
  }
`

interface IconButtonSideTextProps {
  textDirection: string
}

// TODO(petemill): Customize button should get its own
// element and not use IconButton so that the outer
// element can be a <button> and we can use :focus-visible
// and not :focus-within which cannot be combined with :focus-visble.

export const IconButtonSideText = styled('label')<IconButtonSideTextProps>`
  display: grid;
  grid-template-columns: auto auto;
  align-items: center;
  margin-right: ${p => p.textDirection === 'ltr' && '24px'};
  margin-left: ${p => p.textDirection === 'rtl' && '24px'};
  color: inherit;
  cursor: pointer;
  user-select: none;
  width: max-content;

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

interface IconButtonContainerProps {
  textDirection: string
}

export const IconButtonContainer = styled('div')<IconButtonContainerProps>`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 13px;
  font-weight: 600;
  color: rgba(255,255,255,0.8);
  margin-right: ${p => p.textDirection === 'ltr' && '8px'};
  margin-left: ${p => p.textDirection === 'rtl' && '8px'};
  border-right: ${p => p.textDirection === 'ltr' && '1px solid rgba(255, 255, 255, 0.6)'};
  border-left: ${p => p.textDirection === 'rtl' && '1px solid rgba(255, 255, 255, 0.6)'};

  &:hover {
    color: #ffffff;
  }
`
