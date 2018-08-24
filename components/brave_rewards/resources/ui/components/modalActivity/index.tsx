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
  StyledBalance,
  StyledWarning,
  StyledWarningText,
  StyledTables,
  StyledNote,
  StyledTableTitle,
  StyledTableSubTitle,
  StyledVerified,
  StyledVerifiedText,
  StyledSelectOption,
  StyledIcon,
  StyledClosing,
  StyledActionIcon,
  StyledAlertWrapper,
  StyledWarningWrapper,
  StyledVerifiedIcon
} from './style'
import TableContribute, { DetailRow as ContributeRow } from '../tableContribute'
import TableTransactions, { DetailRow as TransactionRow } from '../tableTransactions'
import { Select, ControlWrapper, Modal } from '../../../components'
import { AlertCircleIcon, DownloadIcon, PrintIcon, VerifiedFillIcon } from '../../../components/icons'
import ListToken from '../listToken'
import { Type as TokenType } from '../tokens'

// Utils
import { getLocale } from '../../../helpers'

export interface Token {
  value: number
  converted: number
  isNegative?: boolean
}

export type SummaryType = 'deposit' | 'grant' | 'ads' | 'contribute' | 'recurring' | 'donations'

export interface SummaryItem {
  type: SummaryType
  token: Token
  text: string
  notPaid?: boolean
}

export interface Props {
  contributeRows: ContributeRow[]
  onClose: () => void
  onPrint: () => void
  onDownloadPDF: () => void
  onMonthChange: (value: string, child: React.ReactNode) => void
  months: Record<string, string>
  currentMonth: string
  transactionRows: TransactionRow[]
  openBalance?: Token
  closingBalance?: Token
  id?: string
  summary: SummaryItem[]
  total: Token
  paymentDay: number
}

export default class ModalActivity extends React.PureComponent<Props, {}> {
  private colors: Record<SummaryType, TokenType> = {
    deposit: 'earnings',
    grant: 'earnings',
    ads: 'earnings',
    contribute: 'contribute',
    recurring: 'donation',
    donations: 'donation'
  }

  private hasWarnings: boolean = false

  get headers () {
    return [
      getLocale('rewardsContributeVisited'),
      getLocale('rewardsContributeAttention'),
      getLocale('payment')
    ]
  }

  get selectTitle () {
    return (
      <StyledTitle>
        {getLocale('braveRewards')} <StyledSubTitle>{getLocale('walletActivity')}</StyledSubTitle>
      </StyledTitle>
    )
  }

  getSummaryBox = () => {
    this.hasWarnings = false
    let items: React.ReactNode[]

    if (!this.props.summary) {
      return null
    }

    items = this.props.summary.map((item: SummaryItem, i: number) => {
      let title: React.ReactNode = item.text

      if (item.notPaid) {
        this.hasWarnings = true
        title = (
          <StyledWarningWrapper>
            {title} <StyledAlertWrapper><AlertCircleIcon /></StyledAlertWrapper>
          </StyledWarningWrapper>
        )
      }

      return (
        <ListToken
          key={`${this.props.id}-summary-${i}`}
          title={title}
          value={item.token.value}
          converted={item.token.converted}
          color={item.notPaid ? 'notPaid' : this.colors[item.type]}
          size={'small'}
          border={i === 0 ? 'first' : 'default'}
          isNegative={item.token.isNegative}
        />
      )
    })

    items.push(
      <ListToken
        key={`${this.props.id}-summary-99`}
        title={<b>{getLocale('total')}</b>}
        value={this.props.total.value}
        converted={this.props.total.converted}
        size={'small'}
        border={'last'}
      />
    )

    return items
  }

  render () {
    const {
      id,
      onClose,
      contributeRows,
      onMonthChange,
      currentMonth,
      openBalance,
      closingBalance,
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
                ? <ControlWrapper text={this.selectTitle}>
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
              {
                openBalance && closingBalance
                ? <StyledBalance>
                  <ListToken
                    title={getLocale('openBalance')}
                    value={openBalance.value}
                    converted={openBalance.converted}
                    color={'earnings'}
                    border={'last'}
                  />
                  <StyledClosing>
                    <ListToken
                      title={<b>{getLocale('closeBalance')}</b>}
                      value={closingBalance.value}
                      converted={closingBalance.converted}
                      color={'contribute'}
                      border={'last'}
                    />
                  </StyledClosing>
                </StyledBalance>
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
                <StyledIcon>
                  <StyledActionIcon>
                    <DownloadIcon />
                  </StyledActionIcon>
                  <StyledIconText>{getLocale('downloadPDF')}</StyledIconText>
                </StyledIcon>
              </StyledIconWrap>
              {this.getSummaryBox()}
            </StyledRight>
          </StyledHeader>
          {
            this.hasWarnings
            ? <StyledWarning>
                <StyledAlertWrapper>
                  <AlertCircleIcon />
                </StyledAlertWrapper>
                <StyledWarningText>
                  <b>{getLocale('paymentNotMade')}</b> {getLocale('paymentWarning')}
                </StyledWarningText>
            </StyledWarning>
            : null
          }
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
                <VerifiedFillIcon />
              </StyledVerifiedIcon>
              <StyledVerifiedText>{getLocale('braveVerified')}</StyledVerifiedText>
            </StyledVerified>
          </StyledTables>
          <StyledNote>
            <b>{getLocale('pleaseNote')}</b> {getLocale('activityNote')}
            <br/><br/>
            {getLocale('activityCopy')}
          </StyledNote>
        </StyledWrapper>
      </Modal>
    )
  }
}
