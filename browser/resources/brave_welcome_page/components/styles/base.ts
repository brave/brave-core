/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, spacing } from '@brave/leo/tokens/css/variables'

import bgLight from '../img/bg-light.jpg'
import bgDark from '../img/bg-dark.jpg'
import bgLightVideo from '../img/bg-light.mp4'
import bgDarkVideo from '../img/bg-dark.mp4'

export { bgLightVideo, bgDarkVideo }

export const baseStyles = `
  /* CSS Reset */
  *,
  *::before,
  *::after {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
  }

  img,
  picture,
  video,
  canvas,
  svg {
    display: block;
    max-width: 100%;
  }

  input,
  button,
  textarea,
  select {
    font: inherit;
  }

  p,
  h1,
  h2,
  h3,
  h4,
  h5,
  h6 {
    overflow-wrap: break-word;
  }

  a {
    color: inherit;
    text-decoration: underline;
  }

  ul,
  ol {
    list-style: none;
  }

  & {
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    font: ${font.large.regular};
    color: ${color.text.primary};
    background-color: ${color.primitive.neutral['0']};
    background-image: url(${bgLight});
    background-size: cover;
    background-position: center center;
    background-repeat: no-repeat;
    padding: ${spacing.xl};
    overflow: hidden;
  }

  @media (prefers-color-scheme: dark) {
    & {
      background-image: url(${bgDark});
    }
  }

  .video-background {
    position: absolute;
    top: 50%;
    left: 50%;
    min-width: 100%;
    min-height: 100%;
    width: auto;
    height: auto;
    transform: translate(-50%, -50%);
    z-index: -1;
    object-fit: cover;
  }

  .video-background.light {
    display: block;
  }

  .video-background.dark {
    display: none;
  }

  @media (prefers-color-scheme: dark) {
    .video-background.light {
      display: none;
    }

    .video-background.dark {
      display: block;
    }
  }

  h1 {
    font: ${font.heading.h1};
    margin: 0;
    margin-bottom: ${spacing['2Xl']};
  }

  p {
    margin-bottom: ${spacing['l']};
  }
`

