/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'

import { scopedCSS, css } from '../lib/scoped_css'
import { addStyles } from '../lib/style_injector'

import backgroundAnimated from '../assets/background_animated.svg'
import backgroundStatic from '../assets/background_static.svg'
import backgroundAnimatedDark from '../assets/background_animated_dark.svg'
import backgroundStaticDark from '../assets/background_static_dark.svg'

import panelBackground from '../assets/panel_background.svg'
import panelBackgroundDark from '../assets/panel_background_dark.svg'

export const style = scopedCSS('app', css`
  & {
    --onboarding-max-width: 360px;

    container: app/normal;

    @media (width < 400px) {
      --is-narrow-view: 1;
    }
  }

  &.is-bubble {
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

  .panel-background {
    position: fixed;
    inset: 0;
    z-index: -1;
    background-image: url(${panelBackground});
    background-repeat: no-repeat;
    background-position: center 15px;
    background-size: 392px auto;

    @media (prefers-color-scheme: dark) {
      background-image: url(${panelBackgroundDark});
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
    animation-timing-function: ease-in-out;
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

addStyles('app-global-styles', css`
  ${style.selector} {
    a {
      color: ${color.text.interactive};
    }
  }
`)
