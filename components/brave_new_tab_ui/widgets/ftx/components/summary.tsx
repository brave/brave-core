// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../common/locale'
import {
  ShowIcon,
  HideIcon
} from '../../../components/default/exchangeWidget/shared-assets'
import * as S from '../../shared/styles'
import IconAsset from '../../shared/iconAsset'
import getFormattedPrice from '../../shared/getFormattedPrice'
import * as FTXActions from '../ftx_actions'
import { FTXState } from '../ftx_state'

type Props = {
  ftx: FTXState
  actions: typeof FTXActions
  hideBalance: boolean
  onToggleBalanceVisibility: () => unknown
}

export default function Summary (props: Props) {
  const total = props.ftx.balanceTotal || 0
  const balanceKeys = Object.keys(props.ftx.balances)
  return (
    <S.Box $mt={10}>
      <S.FlexItem isFlex={true} $p={15} hasPadding={true} >
        <S.FlexItem>
          <S.Balance hideBalance={props.hideBalance}>
            <S.Text lineHeight={1.15} $fontSize={21}>{getFormattedPrice(total)}</S.Text>
          </S.Balance>
        </S.FlexItem>
        <S.FlexItem>
          <S.BlurIcon
            onClick={props.onToggleBalanceVisibility}
            aria-label={getLocale(props.hideBalance ? 'ftxSummaryRevealLabel' : 'ftxSummaryBlurLabel')}
          >
            {
              props.hideBalance
              ? <ShowIcon />
              : <HideIcon />
            }
          </S.BlurIcon>
        </S.FlexItem>
      </S.FlexItem>
      {balanceKeys.length !== 0
      ?
      <S.List hasBorder={false}>
        {balanceKeys.map(currencyKey => {
          const balance = props.ftx.balances[currencyKey]
          return (
            <S.ListItem key={currencyKey} isFlex={true} $height={40}>
              <S.FlexItem $pl={5} $pr={5}>
                <IconAsset iconKey={currencyKey.toLowerCase()} size={18} />
              </S.FlexItem>
              <S.FlexItem>
                <S.Text>{currencyKey}</S.Text>
              </S.FlexItem>
              <S.FlexItem textAlign='right' flex={1}>
                <S.Balance hideBalance={props.hideBalance}>
                  <S.Text lineHeight={1.15}>{balance}</S.Text>
                </S.Balance>
              </S.FlexItem>
            </S.ListItem>
          )
        })}
      </S.List>
      : <S.Balance hideBalance={props.hideBalance}>
          <S.Text lineHeight={1.15} $p={12}>{getLocale('ftxSummaryNoBalance')}</S.Text>
        </S.Balance>
      }
    </S.Box>
  )
}
