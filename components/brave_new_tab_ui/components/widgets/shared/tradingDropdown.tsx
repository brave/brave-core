// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  TradeWrapper,
  InputWrapper,
  AmountInputField,
  Dropdown,
  ActionsWrapper,
  ActionButton,
  AssetDropdownLabel,
  AssetItems,
  AssetItem,
  DropdownIcon,
  CaratDropdown,
  BasicBox
} from './styles'
import { CaratDownIcon } from 'brave-ui/components/icons'
import icons from '../shared/assets/icons'

const renderIconAsset = (key: string) => {
  if (!(key in icons)) {
    return null
  }

  return (
    <>
      <img height={20} src={icons[key]} />
    </>
  )
}

interface Props {
  assets: Array<string>
}

export const TradingDropdown = ({
  assets
}: Props) => {
  const [fromDropdownShowing, setFromDropdown] = React.useState(false)
  const [toDropdownShowing, setToDropdown] = React.useState(false)
  const [currentQuantity, setCurrentQuantity] = React.useState('')
  const [fromAsset, setFromAsset] = React.useState('BTC')
  const [toAsset, setToAsset] = React.useState('BAT')

  const toggleDropDowns = (dropdown: string) => {
    if (dropdown === 'to') {
      setToDropdown(!toDropdownShowing)
    } else {
      setFromDropdown(!fromDropdownShowing)
    }
  }

  const handleAssetClick = (asset: string, direction: string) => {
    if (direction === 'to') {
      setToAsset(asset)
    } else {
      setFromAsset(asset)
    }
    setFromDropdown(false)
    setToDropdown(false)
  }

  const handleChange = ({ target }: any) => {
    const { value } = target
    setCurrentQuantity(value)
  }

  return (
    <>
      <TradeWrapper>
        <InputWrapper isFlex={true} itemsShowing={fromDropdownShowing}>
          <AmountInputField
            type={'text'}
            placeholder={`I want to convert...`}
            value={currentQuantity}
            onChange={handleChange}
          />
          <Dropdown
            disabled={false}
            itemsShowing={false}
            className={'asset-dropdown'}
            onClick={toggleDropDowns.bind(null, 'from')}
          >
            {fromAsset}
            <CaratDropdown>
              <CaratDownIcon />
            </CaratDropdown>
          </Dropdown>
          {
            fromDropdownShowing
              ? <AssetItems>
                {assets.filter(v => v !== fromAsset).map((asset: string, i: number, filteredAssets: string[]) => {
                  return (
                    <AssetItem
                      key={`choice-${asset}`}
                      isLast={i === filteredAssets.length - 1}
                      onClick={handleAssetClick.bind(null, asset, 'from')}
                    >
                      <DropdownIcon>
                        {renderIconAsset(asset.toLowerCase())}
                      </DropdownIcon>
                      {asset}
                    </AssetItem>
                  )
                })}
              </AssetItems>
              : null
          }
        </InputWrapper>
        <InputWrapper
          isFlex={true}
          itemsShowing={toDropdownShowing}
        >
          <BasicBox
            isFlex={true}
            isFullWidth={true}
            onClick={toggleDropDowns.bind(null, 'to')}
          >
            <DropdownIcon>
              {renderIconAsset(toAsset.toLowerCase())}
            </DropdownIcon>
            <AssetDropdownLabel>
              {toAsset}
            </AssetDropdownLabel>
            <CaratDropdown>
              <CaratDownIcon />
            </CaratDropdown>
          </BasicBox>
          {
            toDropdownShowing
              ? <AssetItems>
                {assets.filter(v => v !== toAsset).map((asset: string, i: number, filteredAssets: string[]) => {
                  return (
                    <AssetItem
                      key={`choice-${asset}`}
                      isLast={i === filteredAssets.length - 1}
                      onClick={handleAssetClick.bind(null, asset, 'to')}
                    >
                      <DropdownIcon>
                        {renderIconAsset(asset.toLowerCase())}
                      </DropdownIcon>
                      {asset}
                    </AssetItem>
                  )
                })}
              </AssetItems>
              : null
          }
        </InputWrapper>
      </TradeWrapper>
      <ActionsWrapper>
        <ActionButton>
          Preview Conversion
        </ActionButton>
      </ActionsWrapper>
    </>
  )
}