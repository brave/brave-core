/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const Page = styled<{}, 'div'>('div')`
  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  position: relative;
  z-index: 3;
  top: 0;
  left: 0;
  display: flex;
  flex: 1;
  flex-direction: column;
  justify-content: space-between;
  height: 100%;
  min-height: 100vh;
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
  position: absolute;
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
  position: absolute;
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
  -webkit-font-smoothing: antialiased;
  box-sizing: border-box;
  font-size: 12px;
  font-family: Muli, sans-serif;
  color: rgba(255, 255, 255, 0.6);
`

export const Navigation = styled<{}, 'nav'>('nav')`
  display: flex;
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
