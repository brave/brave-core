// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CaratDownIcon } from 'brave-ui/components/icons'
import { getLocale } from '../../../common/locale'
import {
  TradeWrapper,
  InputWrapper,
  AmountInputField,
  Dropdown,
  AssetDropdownLabel,
  AssetItems,
  AssetItem,
  DropdownIcon,
  CaratDropdown,
  BasicBox
} from './styles'
import IconAsset from './iconAsset'

const renderIconAsset = (key: string) => {
  return <IconAsset iconKey={key} size={20} />
}

interface Props {
  fromAssets: string[]
  toAssets: string[]
  onChange: (from: string, to: string, quantity: number) => unknown
}

export const TradingDropdown = ({
  fromAssets,
  toAssets,
  onChange
}: Props) => {
  const [fromDropdownShowing, setFromDropdown] = React.useState(false)
  const [toDropdownShowing, setToDropdown] = React.useState(false)
  const [currentQuantity, setCurrentQuantity] = React.useState<string>()
  const [fromAsset, setFromAsset] = React.useState(fromAssets.length ? fromAssets[0] : '')

  const toAssetsFiltered = React.useMemo(() => {
    return toAssets.filter(name => name !== fromAsset)
  }, [toAssets, fromAsset])

  const [toAsset, setToAsset] = React.useState(toAssetsFiltered.length ? toAssetsFiltered[0] : '')

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
    // Validate
    if (Number.isNaN(Number(value))) {
      return
    }
    // Commit
    setCurrentQuantity(value)
  }

  React.useEffect(() => {
    onChange(fromAsset, toAsset, Number(currentQuantity) || 0)
  }, [fromAsset, toAsset, currentQuantity, onChange])

  return (
    <>
      <TradeWrapper>
        <InputWrapper isFlex={true} itemsShowing={fromDropdownShowing}>
          <AmountInputField
            type={'text'}
            placeholder={getLocale('cryptoConvertAmountPlaceholder')}
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
                {fromAssets.filter(v => v !== fromAsset).map((asset: string, i: number, filteredAssets: string[]) => {
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
                {toAssetsFiltered.filter(v => v !== toAsset).map((asset: string, i: number, filteredAssets: string[]) => {
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
    </>
  )
}
