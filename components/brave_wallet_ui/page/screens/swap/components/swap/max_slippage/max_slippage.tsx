// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Styles
import { OptionButton, CustomInput, RadioIcon } from './max_slippage.style'
import {
  Column,
  LeoSquaredButton,
  Row,
  Text,
  VerticalSpace
} from '../../../../../../components/shared/style'

const SUGGESTED_SLIPPAGE = '0.5'
const MAINSTREAM_ASSET_PAIRS_SLIPPAGE = '0.5'
const STABLECOIN_PAIRS_SLIPPAGE = '0.5'
const OTHERS_SLIPPAGE = '1'

interface Props {
  slippageTolerance: string
  onChangeSlippageTolerance: (slippage: string) => void
}

export const MaxSlippage = (props: Props) => {
  const { slippageTolerance, onChangeSlippageTolerance } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // State
  const [customSlippageTolerance, setCustomSlippageTolerance] =
    React.useState<string>(
      slippageTolerance === SUGGESTED_SLIPPAGE ? '' : slippageTolerance
    )
  const [slippageOption, setSlippageOption] = React.useState<
    'custom' | 'suggested'
  >(slippageTolerance === SUGGESTED_SLIPPAGE ? 'suggested' : 'custom')

  // Methods
  const handleUpdateSlippageTolerance = React.useCallback(() => {
    if (slippageOption === 'custom') {
      onChangeSlippageTolerance(customSlippageTolerance)
      return
    }
    onChangeSlippageTolerance(SUGGESTED_SLIPPAGE)
  }, [slippageOption, customSlippageTolerance, onChangeSlippageTolerance])

  return (
    <Column
      fullWidth={true}
      padding='0px 16px'
    >
      <Row
        marginBottom={isPanel ? '14px' : '24px'}
        justifyContent={isPanel ? 'center' : 'flex-start'}
        padding='0px 8px'
      >
        <Text
          textSize={isPanel ? '16px' : '22px'}
          isBold={true}
          textColor='primary'
        >
          {getLocale('braveWalletMaxSlippage')}
        </Text>
      </Row>
      <Column
        padding='0px 8px'
        alignItems='flex-start'
        margin='0px 0px 16px 0px'
      >
        <Text
          isBold={false}
          textSize='12px'
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletMaxSlippageDescription')}
        </Text>
        <VerticalSpace space='8px' />
        <Text
          isBold={true}
          textSize='12px'
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletSuggestedValues')}
        </Text>
        <VerticalSpace space='8px' />
        <Text
          isBold={false}
          textSize='12px'
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletMainstreamAssetPairs').replace(
            '$1',
            MAINSTREAM_ASSET_PAIRS_SLIPPAGE
          )}
        </Text>
        <Text
          isBold={false}
          textSize='12px'
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletStablecoinPairs').replace(
            '$1',
            STABLECOIN_PAIRS_SLIPPAGE
          )}
        </Text>
        <VerticalSpace space='8px' />
        <Text
          isBold={false}
          textSize='12px'
          textColor='primary'
          textAlign='left'
        >
          {getLocale('braveWalletOthers').replace('$1', OTHERS_SLIPPAGE)}
        </Text>
      </Column>
      <Row gap='12px'>
        <OptionButton
          onClick={() => setSlippageOption('suggested')}
          isSelected={slippageOption === 'suggested'}
        >
          <Column alignItems='flex-start' gap='8px'>
            <Text
              isBold={true}
              textSize='12px'
              textColor='primary'
            >
              {getLocale('braveWalletSuggested')}
            </Text>
            <Text
              isBold={true}
              textSize='16px'
              textColor='primary'
            >
              {SUGGESTED_SLIPPAGE}%
            </Text>
          </Column>
          <RadioIcon
            name={
              slippageOption === 'suggested'
                ? 'radio-checked'
                : 'radio-unchecked'
            }
            isSelected={slippageOption === 'suggested'}
          />
        </OptionButton>
        <OptionButton
          onClick={() => setSlippageOption('custom')}
          isSelected={slippageOption === 'custom'}
        >
          <Column alignItems='flex-start' gap='8px'>
            <Text
              isBold={true}
              textSize='12px'
              textColor='primary'
            >
              {getLocale('braveWalletCustom')}
            </Text>
            <Row width='unset'>
              <CustomInput
                type='text'
                value={customSlippageTolerance}
                onInput={(e) => setCustomSlippageTolerance(e.value)}
                disabled={slippageOption !== 'custom'}
                placeholder='0'
              >
                <Icon
                  name='percent'
                  slot='right-icon'
                />
              </CustomInput>
            </Row>
          </Column>
          <RadioIcon
            name={
              slippageOption === 'custom' ? 'radio-checked' : 'radio-unchecked'
            }
            isSelected={slippageOption === 'custom'}
          />
        </OptionButton>
      </Row>
      <Row padding='16px 0px'>
        <LeoSquaredButton
          onClick={handleUpdateSlippageTolerance}
          size='large'
          isDisabled={
            slippageOption === 'custom' && customSlippageTolerance === ''
          }
        >
          {getLocale('braveWalletUpdate')}
        </LeoSquaredButton>
      </Row>
    </Column>
  )
}
