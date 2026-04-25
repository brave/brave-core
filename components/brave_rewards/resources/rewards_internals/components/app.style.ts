/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

import rewardsLogoImage from '../../rewards_page/assets/rewards_logo.svg'
import rewardsLogoImageDark from '../../rewards_page/assets/rewards_logo_dark.svg'

const wideWidth = '800px'

export const style = scoped.css`
  & {
    display: flex;
    block-size: 100vh;
    container-type: inline-size;

    @media (prefers-color-scheme: dark) {
      scrollbar-color: rgba(255, 255, 255, 0.25) rgba(0, 0, 0, 0);
    }
  }

  .sidebar {
    min-width: 250px;
    display: flex;
    flex-direction: column;
    background: ${color.container.background};

    @container (width < ${wideWidth}) {
      position: fixed;
      inset-block-start: 0;
      inset-block-end: 0;
      inset-inline-start: 0;
      z-index: 2;
      border-inline-end: solid 1px ${color.divider.subtle};
      box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.08);

      transform: translateX(-110%);
      transition: transform 250ms;

      &.open {
        transform: translateX(0);
      }
    }
  }

  nav {
    flex: 1 0 auto;
    overflow: auto;
  }

  .page-content {
    flex: 1 1 auto;
    padding: 32px;
    overflow: auto;
    scrollbar-gutter: stable;
    position: relative;
  }

  .sidebar-toggle {
    position: absolute;
    inset-block-start: 24px;
    inset-inline-start: 24px;
    z-index: 1;

    @container (width >= ${wideWidth}) {
      display: none;
    }
  }

  header {
    padding: 40px 24px 16px;
  }

  main {
    margin: 0 auto;
    max-width: 1024px;
    display: flex;
    flex-direction: column;
    gap: 24px;
  }

  h1 {
    padding: 0 32px;
  }

  ul {
    list-style-type: none;
    padding: 0;
    margin: 0;
  }

  li {
    position: relative;

    a, button {
      --leo-icon-size: 18px;

      width: 100%;
      color: ${color.text.secondary};
      font: ${font.components.navbutton};
      text-decoration: none;
      display: flex;
      align-items: center;
      gap: 16px;
      padding: 13px 24px;

      &:hover {
        background: ${color.container.highlight};
      }
    }

    a.current {
      --leo-icon-color: ${color.icon.interactive};
      color: ${color.text.interactive};

      &::before {
        content: '';
        position: absolute;
        inset-block-start: 8px;
        inset-inline-start: 0;
        display: block;
        inline-size: 4px;
        block-size: 32px;
        background: ${color.icon.interactive};
        border-start-end-radius: 2px;
        border-end-end-radius: 2px;
      }
    }
  }

  .disclaimer {
    text-align: center;
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
`

style.passthrough.css`
  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
  }

  h1 {
    margin: 0;
    font: ${font.heading.h3};
    text-align: center;
  }

  h3 {
    font: ${font.heading.h3};
    margin: 0;
  }

  h4 {
    font: ${font.heading.h4};
    margin: 0;
  }

  .key-value-list {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 8px 16px;

    > div {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 16px;
      padding: 8px 0;

      &:not(:last-child) {
        border-bottom: solid 1px ${color.divider.subtle};
      }
    }
  }

  table {
    flex-grow: 1;
    margin: 8px;

    th {
      text-align: left;
    }

    td, th {
      padding: 8px 4px;
    }

    tbody tr {
      td, th {
        border-top: solid 1px ${color.divider.subtle};
      }
    }
  }

  .content-card {
    border-radius: 16px;
    padding: 4px;
    background-color: rgba(255, 255, 255, 0.55);
    display: flex;
    flex-direction: column;
    gap: 4px;

    section {
      border-radius: 12px;
      background: ${color.container.background};
      width: 100%;
      overflow-x: auto;
    }

    h4 {
      padding: 8px;
    }

    @media (prefers-color-scheme: dark) {
      background-color: rgba(37, 37, 37, 0.58);
    }
  }
`
