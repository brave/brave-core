/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'

export const Header = styled<{}, 'header'>('header')`
  box-sizing: border-box;
  align-items: center;
  display: grid;
  height: 100%;
  grid-template-columns: 1fr auto auto;
  grid-template-rows: 1fr;
  grid-gap: 40px 0;
  grid-template-areas:
    "stats clock"
    "topsites topsites";

  > *:first-child {
    grid-area: stats;
  }

  > *:nth-child(2) {
    grid-area: clock;
  }

  > *:nth-child(3) {
    grid-area: topsites;
  }

  @media screen and (max-width: 904px) {
    grid-template-areas:
    "clock"
    "stats"
    "topsites";

    > *:first-child {
      margin: auto;
      text-align: center;
    }

    > *:nth-child(2) {
      margin: auto;
      text-align: center;
    }

    > *:nth-child(3) {
      justify-content: center;
    }
  }
`

export const Main = styled<{}, 'main'>('main')`
  display: flex;
  justify-content: flex-start;
`

export const Footer = styled<{}, 'footer'>('footer')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  margin-top: 60px;
  align-items: center;
  grid-template-columns: 1fr auto;
  grid-template-rows: auto;
  grid-gap: 0;
  grid-template-areas: "credits actions";

    > *:first-child {
      grid-area: credits;
    }

    > *:nth-child(2) {
      grid-area: actions;
    }

  @media screen and (max-width: 904px) {
    grid-template-areas:
      "credits actions";

    > *:first-child {
      text-align: left;
    }

    > *:nth-child(2) {
      justify-content: flex-end;
    }
  }
`
