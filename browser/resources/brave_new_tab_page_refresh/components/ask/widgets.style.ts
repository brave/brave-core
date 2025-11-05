/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font, radius } from '@brave/leo/tokens/css/variables'
import { scoped } from '../../lib/scoped_css'

export const style = scoped.css`
  @font-face {
    font-family: 'alarm clock';
    src: url('alarm clock.ttf') format('truetype');
    font-weight: normal;
    font-style: normal;
  }

  & {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .content-area {
    display: flex;
    gap: 16px;
    width: 100%;
  }

  .widget {
    width: 240px;
    height: 160px;
    border-radius: ${radius.xl};
    overflow: hidden;
    position: relative;
    flex-shrink: 0;
  }

  /* Privacy Stats Widget */
  .privacy-stats {
    background: ${color.neutral[10]};
    display: flex;
    flex-direction: column;
  }

  .privacy-stats-header {
    padding: 12px;
    display: flex;
    gap: 4px;
    align-items: center;
  }

  .privacy-stats-title {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    flex: 1;
  }

  .privacy-stats-content {
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 8px;
    align-items: flex-start;
    justify-content: flex-end;
  }

  .stat-container {
    background: ${color.container.background};
    padding: 4px 10px;
    display: flex;
    gap: 8px;
    align-items: center;
    justify-content: center;
    width: 100%;
    box-sizing: border-box;

    &.top {
      border-radius: 8px 8px 4px 4px;
    }

    &.middle {
      border-radius: 4px;
    }

    &.bottom {
      border-radius: 4px 4px 8px 8px;
    }
  }

  .stat-value {
    display: flex;
    gap: 2px;
    align-items: center;
  }

  .stat-number {
    font: ${font.heading.h4};
    color: ${color.text.tertiary};
    white-space: nowrap;
  }

  .stat-unit {
    font: ${font.default.semibold};
    color: ${color.text.tertiary};
    white-space: nowrap;
  }

  .stat-label {
    flex: 1;
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    text-align: right;
  }

  /* News Digest Widget */
  .news-digest {
    background: ${color.container.background};
    display: flex;
    flex-direction: column;
    gap: 2px;
    position: relative;
  }

  .news-digest-background {
    position: absolute;
    top:0;
    left:0;
    width: 240px;
    height:240px;
  }

  .news-digest-image {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    object-position: 50% 50%;
    pointer-events: none;
  }

  .news-digest-overlay {
    position: absolute;
    inset: 0;
    background: rgba(0, 0, 0, 0.6);
  }

  .news-digest-header {
    padding: 12px;
    display: flex;
    gap: 4px;
    align-items: center;
    height: 40px;
    box-sizing: border-box;
    position: relative;
    z-index: 1;
  }

  .news-digest-title {
    font: ${font.small.regular};
    color: white;
    flex: 1;
  }

  .carousel-dot {
    width: 9px;
    height: 9px;
    border-radius: 50%;
    background: white;
  }

  .news-digest-content {
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 16px;
    align-items: flex-start;
    justify-content: flex-end;
    position: relative;
    z-index: 1;
  }

  .news-headline {
    font: ${font.default.semibold};
    color: white;
    width: 100%;
  }

  .news-footer {
    display: flex;
    gap: 8px;
    align-items: center;
    width: 100%;
  }

  .news-logo-container {
    width: 20px;
    height: 20px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(255, 255, 255, 0.73);
    backdrop-filter: blur(17.5px);
    border-radius: 5px;
  }

  .news-source {
    font: ${font.small.regular};
    color: white;
    flex: 1;
  }

  /* Weather Widget */
  .weather {
    background: linear-gradient(to bottom, #71aaff, #5a85c5);
    position: relative;
  }

  .weather-header {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    padding: 12px;
    box-sizing: border-box;
    display: flex;
    gap: 4px;
    align-items: center;
    height: 40px;
  }

  .weather-location {
    font: ${font.small.regular};
    color: white;
    flex: 1;
  }

  .weather-content {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    padding: 16px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
  }

  .weather-temp {
    font: ${font.heading.h1};
    color: white;
    width: 100%;
  }

  .weather-description {
    font: ${font.small.semibold};
    color: white;
    width: 100%;
  }

  /* Proton Widget */
  .proton {
    background: ${color.container.background};
    position: relative;
  }

  .proton-background {
    position: absolute;
    top:0;
    left:0;
    width:240px;
    height:180px;
    background: rgba(0, 0, 0, 0.3);
  }

  .proton-image {
    position: absolute;
    top:0;
    left:0;
    height:180px;
  }

  .proton-overlay {
    position: absolute;
    top:0;
    left:0;
    width:240px;
    height:180px;
  }

  .proton-content {
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 16px;
    align-items: flex-start;
    justify-content: flex-end;
    position: relative;
    z-index: 1;
    height: 100%;
    box-sizing: border-box;
  }

  .proton-logo {
    height: 24px;
  }

  .proton-text {
    font: ${font.default.semibold};
    color: white;
    width: 100%;
  }

  .proton-button {
    background: #00095B;
    padding: 8px 12px;
    border-radius: 1000px;
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 36px;
    overflow: hidden;

    &:hover {
      opacity: 0.9;
    }
  }

  .proton-button-text {
    font: ${font.small.semibold};
    color: white;
    padding: 4px;
    letter-spacing: -0.4px;
  }

  /* World Clock Widget */
  .world-clock {
    background: black;
    position: relative;
  }

  .clock-outer-ring {
    position: absolute;
    left: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
    width: 232px;
    height: 152px;
    background: linear-gradient(to top, #4b4b4b, #b1b1b1);
    border-radius: 12px;
    box-shadow: inset 0 1px 0 0 rgba(255, 255, 255, 0.2);
  }

  .clock-inner-ring {
    position: absolute;
    left: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
    width: 228px;
    height: 148px;
    background: #080808;
    border: 1px solid rgba(255, 255, 255, 0.13);
    border-radius: 10px;
  }

  .clock-reflection {
    position: absolute;
    left: 24px;
    top: 5.51px;
    width: 122px;
    height: 8.492px;
    opacity: 0.5;
  }

  .clock-display {
    position: absolute;
    left: 50%;
    top: calc(50% - 24px);
    transform: translate(-50%, -50%);
    width: 210px;
    height: 82px;
    background: black;
    border-radius: 4px;
  }

  .clock-display-gradient {
    position: absolute;
    left: 50%;
    top: calc(50% - 24px);
    transform: translate(-50%, -50%);
    width: 208px;
    height: 80px;
    background: linear-gradient(to bottom, #390407, #240002 50%, #390407);
    border: 1px solid #110203;
    border-radius: 4px;
    box-shadow: inset 0 4px 5px 0 #4d0b0f, inset 0 1px 23px 0 #410004;
  }

  .clock-location-label {
    position: absolute;
    left: 50%;
    top: 78px;
    transform: translate(-50%, -50%);
    width: 208px;
    font: ${font.default.regular};
    color: white;
    opacity: 0.75;
    text-align: center;
  }

  .clock-time {
    font-family: 'alarm clock', sans-serif;
    font-size: 40px;
    color: #ff4a54;
    text-shadow: #ff1d2a 0 0 4px, rgba(0, 0, 0, 0.25) 0 4px 4px;
    white-space: nowrap;
    line-height: normal;
  }

  .clock-main-time {
    position: absolute;
    left: 0;
    top: 0;
  }

  .clock-main-hours {
    position: absolute;
    left: 86.5px;
    top: 47.5px;
    transform: translate(-50%, -50%);
  }

  .clock-main-period {
    position: absolute;
    left: 186.5px;
    top: 47.5px;
    transform: translate(-50%, -50%);
  }

  .clock-shadow {
    font-family: 'alarm clock', sans-serif;
    font-size: 44px;
    color: #e4000d;
    opacity: 0.15;
    white-space: nowrap;
    line-height: normal;
  }

  .clock-secondary-time {
    position: absolute;
    font-family: 'alarm clock', sans-serif;
    font-size: 16px;
    color: #e4000d;
    white-space: nowrap;
    line-height: normal;
    left: 24px;
  }

  .clock-secondary-time-shadow {
    position: absolute;
    font-family: 'alarm clock', sans-serif;
    font-size: 16px;
    color: #e4000d;
    opacity: 0.2;
    white-space: nowrap;
    line-height: normal;
    left: 24px;
  }

  .clock-time-1 {
    top: 113px;
    transform: translateY(-50%);
  }

  .clock-time-2 {
    top: 135px;
    transform: translateY(-50%);
  }

  .clock-period {
    left: 71px;
  }

  .clock-location {
    position: absolute;
    left: 95px;
    font: ${font.small.regular};
    color: white;
    opacity: 0.75;
    width: 121px;
  }

  /* Empty Widget */
  .empty-widget {
    background: ${color.neutral[10]};
    display: flex;
    flex-direction: column;
  }

  .empty-widget-header {
    padding: 12px;
    display: flex;
    gap: 4px;
    align-items: center;
  }

  .empty-widget-title {
    font: ${font.small.regular};
    color: ${color.text.tertiary};
    flex: 1;
  }

  .empty-widget-content {
    height: 82px;
    width: 100%;
  }
`

