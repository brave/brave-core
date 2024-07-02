/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'

import { addStyles, componentStyle, css } from '../lib/component_style'

import backgroundAnimated from '../assets/background_animated.svg'
import backgroundStatic from '../assets/background_static.svg'
import backgroundAnimatedDark from '../assets/background_animated_dark.svg'
import backgroundStaticDark from '../assets/background_static_dark.svg'

addStyles(css`
  a {
    color: ${color.text.interactive};
  }
`)

export const Style = componentStyle('app', css`
  & {
    container: app/normal;

    --onboarding-max-width: 360px;

    @media (width < 400px) {
      --is-narrow-view: 1;
    }
  }

  .is-bubble & {
    width: 392px;
    min-height: 550px;
  }

  .background {
    position: fixed;
    inset: 0;
    z-index: -1;
    background-image: url(${backgroundStatic});
    background-size: cover;

    .animated-background & {
      background-image: url(${backgroundAnimated});
    }

    @media (prefers-color-scheme: dark) {
      background-image: url(${backgroundStaticDark});

      .animated-background & {
        background-image: url(${backgroundAnimatedDark});
      }
    }
  }

  .loading {
    position: fixed;
    inset: 0 0 10% 0;
    display: flex;
    align-items: center;
    justify-content: center;
    opacity: 0;
    animation-name: app-loading-fade-in;
    animation-easing-function: ease-in-out;
    animation-duration: 500ms;
    animation-delay: 1.5s;
    animation-fill-mode: both;

    --leo-progressring-size: 32px
  }

  @keyframes app-loading-fade-in {
    from { opacity: 0; }
    to { opacity: 1; }
  }
`)
