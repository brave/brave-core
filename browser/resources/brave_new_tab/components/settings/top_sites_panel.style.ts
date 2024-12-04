/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

import favoritesActive from '../../assets/favorites_active.svg'
import favoritesActiveDark from '../../assets/favorites_active_dark.svg'
import favoritesInactive from '../../assets/favorites_inactive.svg'
import favoritesInactiveDark from '../../assets/favorites_inactive_dark.svg'
import frequentlyVisitedActive from '../../assets/frequently_visited_active.svg'
import frequentlyVisitedActiveDark from '../../assets/frequently_visited_active_dark.svg'
import frequentlyVisitedInactive from '../../assets/frequently_visited_inactive.svg'
import frequentlyVisitedInactiveDark from '../../assets/frequently_visited_inactive_dark.svg'

export const style = scoped.css`
  & {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .toggle-row {
    display: flex;
    align-items: center;

    label {
      flex: 1 1 auto;
    }
  }

  .list-view-options {
    display: flex;
    flex-wrap: wrap;
    gap: 24px;
    align-items: flex-start;

    > * {
      flex: 1 1 190px;
      display: flex;
      flex-direction: column;
    }
  }

  h4 {
    margin-top: 8px;
  }

  .list-view-image {
    width: 100%;
    flex-basis: 146px;
    border-radius: 8px;
    border: solid 1px ${color.divider.subtle};
    padding: 16px;
    background-position: center;
    background-repeat: no-repeat;
    background-size: auto 114px;
    position: relative;

    .active & {
      border: solid 2px ${color.icon.interactive};
      background-color: ${color.primary['10']};
    }
  }

  .custom {
    .list-view-image {
      background-image: url(${favoritesInactive});
      @media (prefers-color-scheme: dark) {
        background-image: url(${favoritesInactiveDark});
      }
    }

    &.active .list-view-image {
      background-image: url(${favoritesActive});
      @media (prefers-color-scheme: dark) {
        background-image: url(${favoritesActiveDark});
      }
    }
  }

  .most-visited {
    .list-view-image {
      background-image: url(${frequentlyVisitedInactive});
      @media (prefers-color-scheme: dark) {
        background-image: url(${frequentlyVisitedInactiveDark});
      }
    }

    &.active .list-view-image {
      background-image: url(${frequentlyVisitedActive});
      @media (prefers-color-scheme: dark) {
        background-image: url(${frequentlyVisitedActiveDark});
      }
    }
  }
`
