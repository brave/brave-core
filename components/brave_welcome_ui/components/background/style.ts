// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import hillBgUrl from '../../assets/hill.webp'
import pyramidBgUrl from '../../assets/pyramid.webp'

export const Box = styled.div`
  .content-box {
    position: fixed;
    width: 100%;
    height: 100%;
    z-index: 999;

    display: flex;
    align-items: center;
    justify-content: center;
  }

  .background-img {
    position: fixed;
    width: 100%;
    height: 100%;
    z-index: 1;
    object-fit: cover;
    opacity: 0;
    transition: opacity .2s ease-in;

    &.is-visible {
      opacity: 1;
    }
  }

  .hills-container {
    position: fixed;
    width: 100vw;
    height: 100%;
    z-index: 2;
    opacity: 0;
  }

  .stars-container {
    position: fixed;
    width: 100vw;
    height: 100%;
    z-index: 50;
    opacity: 0;

    svg {
      width: 100%;
      height: auto;
      position: absolute;
      transform-origin: center;
    }

    .stars01 {
      bottom: 0;
      transform: scale(1.14);
      filter: blur(3px);
    }

    .stars02 {
      top: 10%;
    }

    .stars03 {
      top: 15%;
    }

    .stars04 {
      top: 30%;
      transform: scale(0.8);
      opacity: 0;
    }
  }

  .hills-base {
    width: 100%;
    height: 100%;
    background: url(${hillBgUrl}) no-repeat;
    background-size: contain;
    background-position-y: bottom;
    position: absolute;
    top: 0;

    &.hills01 {
      z-index: 4;
      transform-origin: bottom;
      background-position-x: -320px;
      transform: scale(1.5);
    }

    &.hills02 {
      z-index: 3;
      transform-origin: bottom right;
    }

    &.hills03 {
      z-index: 2;
      transform-origin: bottom right;
    }
  }

  .pyramid {
    width: 100%;
    height: 100%;
    background: url(${pyramidBgUrl}) no-repeat;
    background-size: 20%;
    background-position: bottom right;
    position: absolute;
    top: 0;
    z-index: 1;
    transform: translateX(20%);
   }
`
