// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Tooltip from '@brave/leo/react/tooltip'

// Types
import { BraveWallet } from '../../../../../../constants/types'

// Hooks
import { useDominantColorFromImageURL } from './use_dominant_color_from_image_url'

// Components
import { LoadingSkeleton } from '../../../../../shared/loading-skeleton'

// Utils
import { getLocale } from '../../../../../../../common/locale'
import { getAssetIdKey } from '../../../../../../utils/asset-utils'

// Selectors
import { useSafeUISelector } from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Styled Components
import {
  Title,
  Segment,
  SegmentBar,
  SegmentMetaRow,
  SegmentSymbol,
  SegmentPercent,
  SegmentFiat,
} from './portfolio_overview_distribution.style'
import { Column, Row } from '../../../../../shared/style'
import styles from './style.module.scss'

// iOS WebKit blocks sampling remote images to canvas; use fixed segment hues instead.
const IOS_DISTRIBUTION_SEGMENT_COLORS: string[] = [
  '#7986CB',
  '#4DB6AC',
  '#FF8A65',
]

const OTHER_SEGMENT_BAR_COLOR = '#9E9E9E'

/**
 * One row of the portfolio “Assets distribution” bar: share of total portfolio fiat and
 * labels for a flex segment. Built in `portfolio-overview.tsx` and passed to
 * {@link PortfolioOverviewDistribution}.
 *
 * Discriminated by `kind`:
 * - **`asset`**: A top-held token (by fiat). `token` supplies symbol, logo, and stable React keys.
 * - **`other`**: The combined remainder after the top slice; no `token`; UI uses a fixed bar color.
 *
 * `value` is the segment’s percentage of the portfolio (0–100). `fiatValue` is the pre-formatted
 * fiat string for that slice.
 */
export type PortfolioOverviewDistributionData =
  | {
      kind: 'asset'
      token: BraveWallet.BlockchainToken
      value: number
      fiatValue: string
    }
  | {
      kind: 'other'
      value: number
      fiatValue: string
    }

function DistributionSegmentBar(props: {
  item: PortfolioOverviewDistributionData
  segmentIndex: number
}) {
  // Props
  const { item, segmentIndex } = props

  // Computed
  const barColor = item.kind === 'other' ? OTHER_SEGMENT_BAR_COLOR : undefined
  const logoUrl = item.kind === 'asset' ? item.token.logo : undefined
  const isIOS = useSafeUISelector(UISelectors.isIOS)
  const useIosPlaceholder = isIOS && !barColor && Boolean(logoUrl)
  const logoSrcForDominant = useIosPlaceholder || barColor ? undefined : logoUrl
  const dominantFromLogo = useDominantColorFromImageURL(logoSrcForDominant, 1)
  const iosPlaceholderColor = useIosPlaceholder
    ? IOS_DISTRIBUTION_SEGMENT_COLORS[
        segmentIndex % IOS_DISTRIBUTION_SEGMENT_COLORS.length
      ]
    : undefined
  const resolvedColor = barColor ?? iosPlaceholderColor ?? dominantFromLogo

  return resolvedColor ? (
    <SegmentBar $color={resolvedColor} />
  ) : (
    <LoadingSkeleton
      width='100%'
      height='8px'
      borderRadius='16px'
      inline={true}
    />
  )
}

interface Props {
  readonly data: PortfolioOverviewDistributionData[]
}

export function PortfolioOverviewDistribution({ data }: Props) {
  // State
  const [tooltipId, setTooltipId] = React.useState<string | undefined>(
    undefined,
  )
  if (data.length === 0) {
    return null
  }

  return (
    <Column
      padding='16px'
      gap='12px'
      width='100%'
      alignItems='flex-start'
    >
      <Title textColor='tertiary'>
        {getLocale('braveWalletPortfolioAssetsDistributionTitle')}
      </Title>
      <Row
        gap='8px'
        justifyContent='flex-start'
        alignItems='flex-start'
      >
        {data.map((item, segmentIndex) => {
          const key =
            item.kind === 'other'
              ? 'distribution-other'
              : getAssetIdKey(item.token)
          const label =
            item.kind === 'asset'
              ? item.token.symbol
              : getLocale('braveWalletTransactionTypeNameOther')
          const percentage = item.value.toFixed()
          return (
            <Segment
              key={key}
              className={styles.portfolioOverviewDistributionSegment}
              $grow={item.value}
              alignItems='center'
              gap='4px'
              onMouseEnter={() => setTooltipId(key)}
              onMouseLeave={() => setTooltipId(undefined)}
            >
              <Tooltip
                text={`${label} (${percentage}%) — ${item.fiatValue}`}
                visible={tooltipId === key}
                placement='top'
              />
              <DistributionSegmentBar
                item={item}
                segmentIndex={segmentIndex}
              />
              <SegmentMetaRow
                justifyContent='space-between'
                alignItems='flex-start'
                gap='4px'
              >
                <SegmentSymbol textColor='tertiary'>{label}</SegmentSymbol>
                {item.value ? (
                  <SegmentPercent
                    textSize='12px'
                    textColor='tertiary'
                  >
                    {percentage}%
                  </SegmentPercent>
                ) : (
                  <LoadingSkeleton
                    width='40px'
                    height='18px'
                    borderRadius='4px'
                    inline={true}
                  />
                )}
              </SegmentMetaRow>
              <SegmentMetaRow
                justifyContent='flex-start'
                alignItems='flex-start'
              >
                {item.fiatValue ? (
                  <SegmentFiat
                    className={styles.portfolioOverviewDistributionFiat}
                    textSize='12px'
                    textColor='primary'
                    textAlign='left'
                    isBold
                  >
                    {item.fiatValue}
                  </SegmentFiat>
                ) : (
                  <LoadingSkeleton
                    width='60px'
                    height='18px'
                    borderRadius='4px'
                    inline={true}
                  />
                )}
              </SegmentMetaRow>
            </Segment>
          )
        })}
      </Row>
    </Column>
  )
}
