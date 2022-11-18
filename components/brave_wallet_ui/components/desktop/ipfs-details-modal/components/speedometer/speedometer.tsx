// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { RadialBarChart, RadialBar, ResponsiveContainer } from 'recharts'
import { Row } from '../../../../shared/style'
import {
  DetailsWrapper,
  SpeedometerWrapper,
  SpeedText,
  SpeedUnitText
} from './speedometer.style'

// const rotation = (n: number) => (0.5 + (1 - n)) * Math.PI
// const circumference = (n: number) => n * 2 * Math.PI

interface Props {
  title: string
  total: number
  filled: number
  noSpeed?: boolean
  color?: string
}

export const Speedometer = ({ title, total, filled, noSpeed, color }: Props) => {
  // const doughnut = {
  //   options: {
  //     legend: {
  //       display: false
  //     },
  //     tooltips: {
  //       enabled: false
  //     },
  //     maintainAspectRatio: false,
  //     cutoutPercentage: 80,
  //     rotation: rotation(0.7),
  //     circumference: circumference(0.7)
  //   },
  //   data: {
  //     labels: ['Speed', 'Nothing'],
  //     datasets: [{
  //       data: [filled, filled > total ? 0 : total - filled],
  //       backgroundColor: [color, '#DEDEDE'],
  //       hoverBackgroundColor: [color, '#DEDEDE'],
  //       borderWidth: [0, 0]
  //     }]
  //   }
  // }

  // network bandwidth units matchin 'ipfs stats bw'
  // to align with what ISP usually shows on invoice
  // const data = filesize(filled, {
  //   standard: 'iec',
  //   base: 2,
  //   output: 'array',
  //   round: 0,
  //   bits: false
  // })
  const data = [
    {
      name: '25-29',
      uv: 26.69,
      pv: 4567,
      fill: '#83a6ed'
    }
  ]
  const speedData = ['40', 'KiB']

  return (
    <SpeedometerWrapper>
      <ResponsiveContainer width={160} height={147}>
        <RadialBarChart
          startAngle={180}
          endAngle={0}
          innerRadius='65%'
          outerRadius='100%'
          data={data}
          >
          <RadialBar
            label={{ position: 'outside', fill: '#fff' }}
            background
            dataKey='uv'
            />
        </RadialBarChart>
      </ResponsiveContainer>

      <DetailsWrapper>
        <Row gap='4px'>
          <SpeedText>{speedData[0]}</SpeedText>
          <SpeedUnitText>{speedData[1]}</SpeedUnitText>
        </Row>
      </DetailsWrapper>
    </SpeedometerWrapper>
  )
}

Speedometer.defaultProps = {
  total: 100,
  filled: 0,
  noSpeed: false,
  color: '#FF6384'
}
