/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../../theme'

export const Header = styled<{}, 'header'>('header')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  margin: 70px 0 0;
  grid-template-columns: 1fr auto auto;
  grid-template-rows: 100px;
  grid-gap: 30px 0;
  grid-template-areas:
    "stats clock"
    "topsites topsites";

  > *:first-child {
    grid-area: stats;
    margin: 0 70px 0 70px;
  }

  > *:nth-child(2) {
    grid-area: clock;
    margin: 0 70px 0 70px;
  }

  > *:nth-child(3) {
    grid-area: topsites;
  }

  @media screen and (max-width: 1150px) {
    grid-template-rows: 1fr;
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
  box-sizing: border-box;
`

export const Footer = styled<{}, 'footer'>('footer')`
  box-sizing: border-box;
  display: grid;
  height: 100%;
  margin: 70px;
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

  @media screen and (max-width: 1150px) {
    margin: 70px 40px;

    grid-template-areas:
      "credits actions";

    > *:first-child {
      text-align: left;
    }

    > *:nth-child(2) {
      justify-content: flex-end;
    }
  }

  @media screen and (max-width: 390px) {
    grid-template-areas:
      "credits"
      "actions";

      > *:first-child {
        text-align: center;
      }

      > *:nth-child(2) {
        justify-content: center;
      }
  }
`
