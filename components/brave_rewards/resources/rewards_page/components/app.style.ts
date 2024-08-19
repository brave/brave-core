/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'

import { scopedCSS, css } from '../lib/scoped_css'
import { addStyles } from '../lib/style_injector'

import backgroundAnimated from '../assets/background_animated.svg'
import backgroundStatic from '../assets/background_static.svg'
import backgroundAnimatedDark from '../assets/background_animated_dark.svg'
import backgroundStaticDark from '../assets/background_static_dark.svg'

import rewardsLogoImage from '../assets/rewards_logo.svg'
import rewardsLogoImageDark from '../assets/rewards_logo_dark.svg'

import selectCaret from '../assets/select_caret.svg'
import selectCaretDark from '../assets/select_caret_dark.svg'

export const style = scopedCSS('app', css`
  & {
    --onboarding-max-width: 392px;
    container: app/normal;
    max-height: 100vh;
    overflow: auto;

    scrollbar-color: #00000040 #0000;
    scrollbar-width: thin;

    @media (prefers-color-scheme: dark) {
      scrollbar-color: #FFFFFF40 #0000;
    }
  }

  &.is-bubble {
    width: 392px;
    min-height: 600px;
    max-height: calc(var(--app-screen-height, 950px) - 80px);
  }

  &.is-narrow-view {
    --is-narrow-view: 1;
  }

  &.is-wide-view {
    --is-wide-view: 1;
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
  @scope (${style.selector}) {
    & {
      font: ${font.default.regular};
      color: ${color.text.primary};
    }

    a {
      color: ${color.text.interactive};
    }

    button {
      margin: 0;
      padding: 0;
      background: 0;
      border: none;
      text-align: unset;
      width: unset;
      font: inherit;
      cursor: pointer;

      &:disabled {
        cursor: default;
      }
    }

    select {
      --select-caret-background-image: url(${selectCaret});
      --select-caret-background-offset: 12px;
      --select-background-color: ${color.container.highlight};

      @media (prefers-color-scheme: dark) {
        --select-caret-background-image: url(${selectCaretDark});
      }

      appearance: none;
      background:
        var(--select-caret-background-image)
          calc(100% - var(--select-caret-background-offset)) center no-repeat,
        var(--select-background-color);
      background-size: 12px;
      border-radius: 8px;
      border: 1px solid color-mix(in srgb, #fff, #1b1b1f 25%);
      color: inherit;
      font: inherit;
      padding: 8px 36px 8px 11px;

      &.subtle {
        --select-background-color: transparent;
        --select-caret-background-offset: 4px;
        border: none;
        border-radius: 4px;
        padding: 0 28px 0 4px;
      }
    }

    leo-toggle:disabled {
      cursor: default;
    }

    ul {
      margin: 0;
      padding: 0;
      list-style-type: none;
    }

    p {
      margin: 0;
    }

    h1 {
      font: ${font.heading.h1};
      margin: 0;
    }

    h2 {
      font: ${font.heading.h2};
      margin: 0;
    }

    h3 {
      font: ${font.heading.h3};
      margin: 0;
    }

    h4 {
      font: ${font.heading.h4};
      margin: 0;
    }

    .content-card {
      border-radius: 16px;
      padding: 4px;
      background-color: rgb(from ${color.container.background} r g b / 55%);
      display: flex;
      flex-direction: column;
      gap: 4px;

      section {
        border-radius: 12px;
        background: ${color.container.background};
        width: 100%;
      }
    }

    .brave-rewards-logo {
      display: inline-block;
      block-size: 28px;
      inline-size: 107px;
      background-image: url(${rewardsLogoImage});
      background-repeat: no-repeat;
      background-size: contain;

      @media (prefers-color-scheme: dark) {
        background-image: url(${rewardsLogoImageDark});
      }
    }
  }
`)
