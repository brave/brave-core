// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'styled-components'

interface BoxProps {
  isTor?: boolean
}

const Box = styled.div<BoxProps>`
  --bg-gr: linear-gradient(180deg, #0C041E -8.41%, #4E21B7 98.85%);

  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  font-family: ${p => p.theme.fontFamily.heading};
  font-weight: 400;
  color: white;
  width: 100%;
  height: 100%;
  min-height: 100vh;
  background: var(--bg-gr);
  position: relative;

  ${p => p.isTor && css`
    --bg-gr: linear-gradient(180deg, #0C041E -9.18%, #5F0D89 98.85%);
  `}
`

const SVGBoxTop = styled.div`
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  overflow: hidden;
  pointer-events: none;
  background-repeat: no-repeat;
  background-size: 100% 291px;
  background-image: url("data:image/svg+xml,%3Csvg width='1496' height='291' fill='none' xmlns='http://www.w3.org/2000/svg'%3E%3Cg opacity='.08' clipPath='url(%23waves_top)' fill='%23fff'%3E%3Cpath d='M-88.855 124.976v-.7C709.035 558.616 1506.92-34.805 2304.81 239.203v5.863C1506.92-30.691 709.035 561.031-88.854 124.976Z'/%3E%3Cpath d='M-88.855 116.631v-.733C709.035 530.15 1506.92-83.674 2304.81 169.664v6.013C1506.92-79.443 709.035 532.599-88.854 116.631Z'/%3E%3Cpath d='M-88.855 108.07v-.733c797.89 393.665 1595.775-241.062 2393.665-8.894v6.13C1506.92-129.411 709.035 503.517-88.854 108.07Z'/%3E%3Cpath d='M-88.855 99.359v-.75C709.035 471.288 1506.92-184.693 2304.81 25.89v6.246C1506.92-180.296 709.035 473.853-88.854 99.36Z'/%3E%3Cpath d='M-88.855 90.515v-.75C709.035 441.174 1506.92-236.376 2304.81-47.697v6.313C1506.92-231.946 709.035 443.772-88.854 90.515Z'/%3E%3Cpath d='M-88.855 81.537v-.766C709.035 410.76 1506.92-288.626 2304.81-122.05v6.346C1506.92-284.146 709.035 413.358-88.854 81.537Z'/%3E%3Cpath d='M-88.855 72.56v-.75C709.035 380.08 1506.92-341.26 2304.81-197.001v6.412C1506.92-336.762 709.035 382.711-88.854 72.559Z'/%3E%3Cpath d='M-88.855 63.515v-.766C709.035 349.266 1506.92-394.16 2304.81-272.287v6.429C1506.92-389.629 709.035 351.897-88.854 63.515Z'/%3E%3Cpath d='M-88.855 54.438v-.783c797.89 264.731 1595.775-500.83 2393.665-401.41v6.446C1506.92-442.645 709.035 321.017-88.854 54.438Z'/%3E%3Cpath d='M-88.855 45.343v-.783C709.035 287.49 1506.92-500.19 2304.81-423.14v6.429C1506.92-495.661 709.035 290.137-88.854 45.343Z'/%3E%3Cpath d='M-88.855 36.3v-.784C709.035 256.692 1506.92-553.057 2304.81-498.492v6.429C1506.92-548.56 709.035 259.307-88.854 36.299Z'/%3E%3Cpath d='M-88.855 27.288v-.716C709.035 226.045 1506.92-605.69 2304.81-573.377v6.362C1506.92-601.193 709.035 228.643-88.854 27.288Z'/%3E%3Cpath d='M-88.855 18.36v-.782C709.035 195.63 1506.92-657.874 2304.81-647.697v6.363C1506.92-653.443 709.035 198.129-88.854 18.361Z'/%3E%3Cpath d='M-88.855 9.5v-.766C709.035 165.55 1506.92-709.524 2304.81-721.283v6.229C1506.92-705.143 709.035 168.148-88.854 9.5Z'/%3E%3Cpath d='M-88.855.755V.023C583.533 114.516 1255.92-503.972 1928.28-721.283H1945C1267.04-510.518 589.102 117.714-88.855.755Z'/%3E%3Cpath d='M-88.855-7.806v-.733C531.572 81.138 1152-453.487 1772.44-721.283h11.28C1159.51-456.536 535.344 83.769-88.854-7.806Z'/%3E%3Cpath d='M-88.855-16.167v-.716C493.073 52.589 1075-427.304 1656.93-721.283h8.87C1080.92-428.987 496.031 54.887-88.854-16.167Z'/%3E%3C/g%3E%3Cdefs%3E%3CclipPath id='waves_top'%3E%3Cpath fill='%23fff' d='M0 0h1496v291H0z'/%3E%3C/clipPath%3E%3C/defs%3E%3C/svg%3E");
`

const SVGBoxBottom = styled(SVGBoxTop)`
  background-position: bottom;
  background-size: 100% 214px;
  background-image: url("data:image/svg+xml,%3Csvg width='1496' height='214' fill='none' xmlns='http://www.w3.org/2000/svg'%3E%3Cg opacity='.08' clipPath='url(%23waves_bottom)' fill='%23fff'%3E%3Cpath d='M1569.5 115.707v.484C1016.72-184.723 463.932 226.405-88.854 36.569v-4.062C463.932 223.554 1016.72-186.42 1569.5 115.707Z'/%3E%3Cpath d='M1569.5 121.488v.508C1016.72-165.002 463.932 260.262-88.854 84.758v-4.166C463.932 257.33 1016.72-166.699 1569.5 121.488Z'/%3E%3Cpath d='M1569.5 127.454v.508c-552.78-272.77-1105.568 166.976-1658.354 6.127v-4.246C463.932 291.949 1016.72-146.55 1569.5 127.454Z'/%3E%3Cpath d='M1569.5 133.454v.519C1016.72-124.234 463.932 330.248-88.854 184.354v-4.327C463.932 327.201 1016.72-126 1569.5 133.454Z'/%3E%3Cpath d='M452.794 214C825.026 133.224 1197.25-25.213 1569.5 139.57v.577C1201.9-21.786 834.332 131.539 466.764 214h-13.97Z'/%3E%3Cpath d='M590.841 214c326.213-91.162 652.429-203.868 978.659-68.233v.531C1246.58 12.775 923.676 122.584 600.78 214h-9.939Z'/%3E%3Cpath d='M697.068 214c290.8-90.643 581.612-175.02 872.432-61.99v.519C1281.41 41.243 993.327 123.669 705.256 214h-8.188Z'/%3E%3Cpath d='M787.368 214c260.712-85.45 521.402-149.944 782.132-55.724v.531C1311.18 66.064 1052.88 129.116 794.566 214h-7.198Z'/%3E%3Cpath d='M867.948 214c233.842-77.949 467.692-127.557 701.552-49.435v.542C1337.8 88.22 1106.13 136.766 874.455 214h-6.507Z'/%3E%3C/g%3E%3Cdefs%3E%3CclipPath id='waves_bottom'%3E%3Cpath fill='%23fff' d='M0 0h1496v214H0z'/%3E%3C/clipPath%3E%3C/defs%3E%3C/svg%3E");
`

interface Props {
  children: React.ReactNode | React.ReactNode[]
  isTor?: boolean
}

function BackgroundView (props: Props) {
  return (
    <Box isTor={props.isTor}>
      <SVGBoxTop />
      {props.children}
      <SVGBoxBottom />
    </Box>
  )
}

export default BackgroundView
