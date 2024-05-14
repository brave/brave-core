// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '$web-common/locale';
import { spacing } from '@brave/leo/tokens/css/variables';
import * as React from 'react';
import styled from 'styled-components';
import Flex from '$web-common/Flex';
import { Title } from './Card';

const Container = styled(Flex)`
  text-align: center;

  padding: ${spacing['3Xl']};
  gap: ${spacing.m};
  color: var(--bn-glass-70);

  & > svg {
    margin-bottom: ${spacing['2Xl']};
    fill: none;
  }

  & > leo-button {
    flex: 0;
  }
`

export default function NoArticles() {
  return <Container align='center' direction='column' justify='center' gap={spacing.m}>
    <svg xmlns="http://www.w3.org/2000/svg" width="99" height="88" viewBox="0 0 99 88" fill="none">
      <path d="M9.5 0.5C4.80558 0.5 1 4.30558 1 9V79C1 83.6944 4.80558 87.5 9.5 87.5H89.5C94.1944 87.5 98 83.6944 98 79V9C98 4.30558 94.1944 0.5 89.5 0.5H9.5Z" stroke="url(#paint0_linear_5051_10730)" />
      <path d="M9.5 13C9.5 10.7909 11.2909 9 13.5 9H85.5C87.7091 9 89.5 10.7909 89.5 13V52C89.5 54.2091 87.7091 56 85.5 56H13.5C11.2909 56 9.5 54.2091 9.5 52V13Z" fill="url(#paint1_linear_5051_10730)" />
      <rect x="9.5" y="69" width="80" height="4" rx="2" fill="url(#paint2_linear_5051_10730)" />
      <rect x="9.5" y="59" width="48" height="2" rx="1" fill="url(#paint3_linear_5051_10730)" />
      <rect x="9.5" y="75" width="64" height="4" rx="2" fill="url(#paint4_linear_5051_10730)" />
      <defs>
        <linearGradient id="paint0_linear_5051_10730" x1="49.5" y1="1" x2="49.5" y2="78.5" gradientUnits="userSpaceOnUse">
          <stop stopColor="white" stopOpacity="0.15" />
          <stop offset="1" stopColor="white" stopOpacity="0" />
        </linearGradient>
        <linearGradient id="paint1_linear_5051_10730" x1="9.5" y1="33" x2="89.5" y2="33" gradientUnits="userSpaceOnUse">
          <stop stopColor="white" stopOpacity="0.03" />
          <stop offset="1" stopColor="white" stopOpacity="0.2" />
        </linearGradient>
        <linearGradient id="paint2_linear_5051_10730" x1="9.5" y1="71.0426" x2="89.5" y2="71.0425" gradientUnits="userSpaceOnUse">
          <stop stopColor="white" stopOpacity="0.03" />
          <stop offset="1" stopColor="white" stopOpacity="0.2" />
        </linearGradient>
        <linearGradient id="paint3_linear_5051_10730" x1="9.5" y1="60.0213" x2="57.5" y2="60.0213" gradientUnits="userSpaceOnUse">
          <stop stopColor="white" stopOpacity="0.03" />
          <stop offset="1" stopColor="white" stopOpacity="0.2" />
        </linearGradient>
        <linearGradient id="paint4_linear_5051_10730" x1="9.5" y1="77.0426" x2="73.5" y2="77.0425" gradientUnits="userSpaceOnUse">
          <stop stopColor="white" stopOpacity="0.03" />
          <stop offset="1" stopColor="white" stopOpacity="0.2" />
        </linearGradient>
      </defs>
    </svg>
    <Title>{getLocale('braveNewsNoArticlesTitle')}</Title>
    <div>
      {getLocale('braveNewsNoArticlesMessage')}
    </div>
  </Container>
}
