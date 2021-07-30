/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import batEcosystemImage from './assets/tour_bat_ecosystem.svg'
import braveAdsImage from './assets/tour_brave_ads.svg'
import batScheduleImage from './assets/tour_bat_schedule.svg'
import acImage from './assets/tour_ac.svg'
import tippingImage from './assets/tour_tipping.svg'
import redeemImage from './assets/tour_redeem.svg'
import completedImage from './assets/tour_completed.svg'

export const root = styled.div`
  font-family: var(--brave-font-heading);
  min-height: 388px;
  text-align: center;
  display: grid;
  grid-template-columns: 1fr;
  grid-template-rows: auto auto 1fr auto 96px;
  grid-template-areas:
    "header"
    "text"
    "graphic"
    "step-links"
    "nav";

  &.tour-wide {
    min-height: 300px;
    padding: 0 20px;
    text-align: left;
    column-gap: 60px;
    grid-template-columns: 1fr 1fr;
    grid-template-rows: auto 1fr 50px;
    grid-template-areas:
      "header graphic"
      "text graphic"
      "nav step-links";
  }
`

export const stepHeader = styled.div`
  grid-area: header;
  font-weight: 600;
  font-size: 18px;
  line-height: 28px;
  color: var(--brave-palette-black);
`

export const stepText = styled.div`
  grid-area: text;
  margin-top: 8px;
  font-size: 14px;
  line-height: 22px;
  color: var(--brave-palette-neutral700);
`

export const stepContent = styled.div`
  text-align: center;
  grid-area: graphic;
  margin: 10px 0 12px;
`

export const stepGraphic = styled.div`
  height: 100%;
  background-repeat: no-repeat;
  background-position: center;
  margin-left: auto;
  margin-right: auto;

  &.tour-graphic-welcome {
    background-image: url('${batEcosystemImage}');
    max-width: 275px;
  }

  &.tour-graphic-ads {
    background-image: url('${braveAdsImage}');
    max-width: 275px;
  }

  &.tour-graphic-schedule {
    background-image: url('${batScheduleImage}');
    max-width: 250px;
  }

  &.tour-graphic-ac {
    background-image: url('${acImage}');
    max-width: 230px;
  }

  &.tour-graphic-tipping {
    background-image: url('${tippingImage}');
    max-width: 230px;
  }

  &.tour-graphic-redeem {
    background-image: url('${redeemImage}');
    max-width: 200px;
  }

  &.tour-graphic-setup {}

  &.tour-graphic-complete {
    background-image: url('${completedImage}');
    background-size: 89px 89px;
  }

  &.tour-graphic-bitflyer {
    padding-top: 40px;

    img {
      width: 100%;
    }

    /* The narrow view does not have space to accomodate the promo image. */
    .tour-narrow & {
      padding-top: 0;
      > * { display: none; }
    }
  }
`

export const stepLinks = styled.div`
  grid-area: step-links;
  align-self: center;
`

export const nav = styled.div`
  grid-area: nav;
  align-self: center;
`
