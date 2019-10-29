/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'brave-ui/theme'

export const Header = styled<{}, 'header'>('header')`
  box-sizing: border-box;
  display: grid;
  margin: 46px 0 0;
  grid-template-columns: fit-content(100%) auto fit-content(100%);
  grid-template-rows: fit-content(100%) fit-content(100%) fit-content(100%) fit-content(100%);
  grid-template-areas:
    "stats . clock"
    ". . clock"
    "topsites . rewards"
    ". . ."
    "notification notification notification";

  > *:nth-child(1) {
    grid-area: stats;
    margin: 0 46px 0 46px;
  }

  > *:nth-child(2) {
    grid-area: clock;
    margin: 0 46px 0 46px;
  }

  > *:nth-child(3) {
    grid-area: rewards;
  }

  > *:nth-child(4) {
    grid-area: topsites;
    margin: 0 46px 0 46px;
    justify-self: start;
    align-items: normal;
  }

  > *:nth-child(5) {
    grid-area: notification;
    justify-self: center;
  }

  @media screen and (max-width: 1150px) {
    grid-row-gap: 4px;
    grid-template-areas:
    "clock clock clock"
    "stats stats stats"
    "topsites topsites topsites"
    "rewards rewards rewards"
    "notification notification notification";


    > *:nth-child(1) {
      margin: auto;
      text-align: center;
    }

    > *:nth-child(2) {
      margin: auto;
      text-align: center;
    }

    > *:nth-child(3) {
      margin: auto;
      justify-content: center;
    }

    > *:nth-child(4) {
      margin: auto;
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
