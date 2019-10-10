/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledContent,
  StyledNum,
  StyledTabWrapper,
  StyledControlWrapper,
  RestoreWrapper
} from './style'
import RestoreSites from '../restoreSites'
import { Tab } from '../'
import Modal from 'brave-ui/components/popupModals/modal/index'
import TableContribute, { DetailRow } from '../tableContribute/index'
import { getLocale } from 'brave-ui/helpers'

export interface Props {
  rows: DetailRow[]
  excludedRows?: DetailRow[]
  onClose: () => void
  onRestore: () => void
  id?: string
  activeTabId?: number
  onTabChange?: () => void
}

export default class ModalContribute extends React.PureComponent<Props, {}> {

  get headers () {
    return [
      getLocale('site'),
      getLocale('rewardsContributeAttention')
    ]
  }

  getTabTitle = (key: string, numSites?: number) => {
    if (numSites === undefined) {
      return `${getLocale(key)}`
    }

    return `${getLocale(key)} (${numSites})`
  }

  getACTable = () => {
    const { rows } = this.props
    const numSites = rows && rows.length || 0

    return (
      <>
        <TableContribute
          header={this.headers}
          rows={rows}
          numSites={numSites}
          allSites={true}
          showRowAmount={true}
          showRemove={true}
        />
      </>
    )
  }

  getExcludedTable = () => {
    const { excludedRows, onRestore } = this.props
    const numExcludedSites = excludedRows && excludedRows.length || 0

    return (
      <>
        <StyledContent>
          {getLocale('rewardsExcludedText1')} <StyledNum>{numExcludedSites}</StyledNum> {getLocale('rewardsExcludedText2')}
        </StyledContent>
        {
          numExcludedSites > 0
          ? <RestoreWrapper>
              <RestoreSites
                showText={false}
                onRestore={onRestore}
                numExcludedSites={numExcludedSites}
              />
            </RestoreWrapper>
          : null
        }
        <TableContribute
          rows={excludedRows}
          isExcluded={true}
          header={[getLocale('site')]}
          numSites={numExcludedSites}
          allSites={true}
          showRemove={true}
        />
      </>
    )
  }

  render () {
    const {
      id,
      onClose,
      rows,
      excludedRows,
      activeTabId,
      onTabChange
    } = this.props
    const numSites = rows && rows.length || 0
    const numExcluded = excludedRows && excludedRows.length || 0

    return (
      <Modal id={id} onClose={onClose} size={'small'}>
        <StyledWrapper>
          <StyledControlWrapper>
            <StyledTitle>
              {getLocale('rewardsContribute')}
            </StyledTitle>
            <StyledTabWrapper>
              <Tab
                type={'contribute'}
                onChange={onTabChange}
                tabIndexSelected={activeTabId}
                tabTitles={[
                  this.getTabTitle('supportedSites', numSites),
                  this.getTabTitle('excludedSites', numExcluded)
                ]}
              />
            </StyledTabWrapper>
          </StyledControlWrapper>
          {
            activeTabId === 0
            ? this.getACTable()
            : this.getExcludedTable()
          }
        </StyledWrapper>
      </Modal>
    )
  }
}
