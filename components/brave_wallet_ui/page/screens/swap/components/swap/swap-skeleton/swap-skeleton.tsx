// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'

// Components
import {
  SwapSectionBox
} from '../../swap-section-box/swap-section-box'
import { Skeleton } from './skeleton'

// Styled Components
import {
  SwapSkeletonWrapper,
  Header,
  BraveLogo,
  Container,
  FlipWrapper,
  FlipBox
} from './swap-skeleton.style'
import {
  Row,
  HorizontalDivider,
  HorizontalSpacer,
  Text,
  HiddenResponsiveRow,
  ShownResponsiveRow
} from '../../shared-swap.styles'

export const SwapSkeleton = () => {
  return (
    <SwapSkeletonWrapper>
      <Header>
        <Row rowHeight='full' verticalAlign='center'>
          <BraveLogo />
          <HiddenResponsiveRow maxWidth={570}>
            <HorizontalDivider
              height={22}
              marginRight={12}
              dividerTheme='darker'
            />
            <Text
              textSize='18px'
              textColor='text02'
              isBold={true}
            >
              {getLocale('braveSwap')}
            </Text>
          </HiddenResponsiveRow>
        </Row>
        <Row>
          <HiddenResponsiveRow maxWidth={570}>
            <Skeleton
              width={40}
              height={40}
              borderRadius={100}
            />
            <HorizontalSpacer size={16} />
            <Skeleton
              width={154}
              height={40}
              borderRadius={48}
            />
            <HorizontalSpacer size={16} />
            <Skeleton
              width={154}
              height={40}
              borderRadius={48}
            />
          </HiddenResponsiveRow>
          <ShownResponsiveRow>
            <Skeleton
              width={68}
              height={32}
              borderRadius={100}
            />
            <HorizontalSpacer size={16} />
            <Skeleton
              width={135}
              height={32}
              borderRadius={48}
            />
          </ShownResponsiveRow>
        </Row>
      </Header>
      <Container>
        <Row
          rowWidth='full'
          horizontalPadding={16}
          verticalPadding={6}
          marginBottom={18}
        >
          <Text isBold={true}>{getLocale('braveSwap')}</Text>
        </Row>
        <SwapSectionBox boxType='primary'>
          <Row rowWidth='full'>
            <Row>
              <Skeleton
                width={124}
                height={60}
                borderRadius={100}
              />
              <HorizontalDivider
                height={28}
                marginLeft={8}
                marginRight={8}
              />
              <HiddenResponsiveRow maxWidth={570}>
                <Skeleton
                  width={46}
                  height={24}
                  borderRadius={4}
                />
              </HiddenResponsiveRow>
              <HorizontalSpacer size={8} />
              <Skeleton
                width={46}
                height={24}
                borderRadius={4}
              />
            </Row>
            <Skeleton
              width={100}
              height={42}
              borderRadius={5}
            />
          </Row>
        </SwapSectionBox>
        <FlipWrapper>
          <FlipBox>
            <Skeleton
              width={40}
              height={40}
              borderRadius={100}
              background='secondary'
            />
          </FlipBox>
        </FlipWrapper>
        <SwapSectionBox boxType='secondary'>
          <Row rowWidth='full'>
            <Skeleton
              width={160}
              height={40}
              borderRadius={100}
              background='secondary'
            />
            <Skeleton
              width={100}
              height={42}
              borderRadius={5}
              background='secondary'
            />
          </Row>
        </SwapSectionBox>
        <Row
          rowWidth='full'
          horizontalAlign='center'
          verticalPadding={16}
        >
          <Skeleton
            height={56}
            borderRadius={48}
            background='secondary'
          />
        </Row>
      </Container>
    </SwapSkeletonWrapper>
  )
}
