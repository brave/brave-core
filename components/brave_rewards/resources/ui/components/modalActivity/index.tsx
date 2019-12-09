/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import {
  StyledWrapper,
  StyledTitle,
  StyledSubTitle,
  StyledHeader,
  StyledLeft,
  StyledRight,
  StyledIconWrap,
  StyledIconText,
  StyledTables,
  StyledNote,
  StyledTableTitle,
  StyledTableSubTitle,
  StyledVerified,
  StyledVerifiedText,
  StyledSelectOption,
  StyledIcon,
  StyledActionIcon,
  StyledVerifiedIcon
} from './style'
import TableContribute, { DetailRow as ContributeRow } from '../tableContribute'
import TableTransactions, { DetailRow as TransactionRow } from '../tableTransactions'
import { Select, ControlWrapper, Modal } from 'brave-ui/components'
import { PrintIcon, VerifiedSIcon } from 'brave-ui/components/icons'
import ListToken from '../listToken'
import { Type as TokenType } from '../tokens'

// Utils
import { getLocale } from 'brave-ui/helpers'

export interface Token {
  value: string
  converted: string
}

export type SummaryType = 'grant' | 'ads' | 'contribute' | 'monthly' | 'tip'

export interface SummaryItem {
  type: SummaryType
  token: Token
}

export interface Props {
  contributeRows: ContributeRow[]
  onClose: () => void
  onPrint: () => void
  onMonthChange: (value: string, child: React.ReactNode) => void
  months: Record<string, string>
  currentMonth: string
  transactionRows: TransactionRow[]
  openBalance?: Token
  closingBalance?: Token
  id?: string
  summary: SummaryItem[]
  paymentDay: number,
  onlyAnonWallet?: boolean
}

export default class ModalActivity extends React.PureComponent<Props, {}> {
  private summary: Record<SummaryType, {color: TokenType, translation: string}> = {
    grant: {
      color: 'earning',
      translation: this.props.onlyAnonWallet ? 'pointGrantClaimed' : 'tokenGrantClaimed',
    },
    ads: {
      color: 'earning',
      translation: 'earningsAds'
    },
    contribute: {
      color: 'contribute',
      translation: 'rewardsContribute',
    },
    monthly: {
      color: 'contribute',
      translation: 'recurringDonations',
    },
    tip: {
      color: 'contribute',
      translation: 'oneTimeDonation',
    }
  }

  get headers () {
    return [
      getLocale('rewardsContributeVisited'),
      getLocale('rewardsContributeAttention'),
      getLocale('payment')
    ]
  }

  get modalTitle () {
    return (
      <StyledTitle>
        {getLocale('braveRewards')} <StyledSubTitle>{getLocale('walletActivity')}</StyledSubTitle>
      </StyledTitle>
    )
  }

  getSummaryBox = () => {
    let items: React.ReactNode[]

    if (!this.props.summary) {
      return null
    }

    items = this.props.summary.map((item: SummaryItem, i: number) => {
      const summaryItem = this.summary[item.type]
      if (!summaryItem) {
        return undefined
      }

      const negative = summaryItem.color === 'contribute'

      return (
        <ListToken
          key={`${this.props.id}-summary-${i}`}
          title={getLocale(summaryItem.translation)}
          value={item.token.value}
          converted={item.token.converted}
          color={summaryItem.color}
          size={'small'}
          border={i === 0 ? 'first' : 'default'}
          isNegative={negative}
        />
      )
    })

    return items
  }

  render () {
    const {
      id,
      onClose,
      contributeRows,
      onMonthChange,
      currentMonth,
      months,
      transactionRows,
      paymentDay
    } = this.props

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledHeader>
            <StyledLeft>
              {
                months
                  ? <ControlWrapper text={this.modalTitle}>
                    <Select
                      value={currentMonth}
                      onChange={onMonthChange}
                    >
                      {
                        Object.keys(months).map((item: string) => {
                          return <div data-value={item} key={`${id}-monthly-${item}`}>
                            <StyledSelectOption>{months[item]}</StyledSelectOption>
                          </div>
                        })
                      }
                    </Select>
                  </ControlWrapper>
                  : null
              }
            </StyledLeft>
            <StyledRight>
              <StyledIconWrap>
                <StyledIcon>
                  <StyledActionIcon>
                    <PrintIcon />
                  </StyledActionIcon>
                  <StyledIconText>{getLocale('print')}</StyledIconText>
                </StyledIcon>
              </StyledIconWrap>
              {this.getSummaryBox()}
            </StyledRight>
          </StyledHeader>
          <StyledTables>
            <StyledTableTitle>{getLocale('transactions')}</StyledTableTitle>
            <TableTransactions
              rows={transactionRows}
            />
            <StyledTableTitle>
              <span>{getLocale('contributeAllocation')}</span>
              <StyledTableSubTitle>
                {getLocale('paymentMonthly', { day: paymentDay })}
              </StyledTableSubTitle>
            </StyledTableTitle>
            <TableContribute
              header={this.headers}
              rows={contributeRows}
              allSites={true}
              showRowAmount={true}
            />
            <StyledVerified>
              <StyledVerifiedIcon>
                <VerifiedSIcon />
              </StyledVerifiedIcon>
              <StyledVerifiedText>{getLocale('braveVerified')}</StyledVerifiedText>
            </StyledVerified>
          </StyledTables>
          <StyledNote>
            <b>{getLocale('pleaseNote')}</b> {getLocale('activityNote')}
            <br /><br />
            ©2016–{new Date().getFullYear()} {getLocale('activityCopy')}
          </StyledNote>
        </StyledWrapper>
      </Modal>
    )
  }
}
