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
  StyledSelectOption,
  StyledIcon,
  StyledActionIcon,
  Tabs,
  Tab,
  TabContent,
  PaymentMonthly
} from './style'
import TableActivity, { DetailRow as ActivityRow } from '../tableActivity'
import TableTransactions, { DetailRow as TransactionRow } from '../tableTransactions'
import { Select, ControlWrapper, Modal } from 'brave-ui/components'
import { PrintIcon } from 'brave-ui/components/icons'
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

export interface ExtendedActivityRow extends ActivityRow {
  type: SummaryType
}

export interface Props {
  activityRows: ExtendedActivityRow[]
  onClose: () => void
  onPrint?: () => void
  onMonthChange: (value: string, child: React.ReactNode) => void
  months: Record<string, string>
  transactionRows: TransactionRow[]
  id?: string
  summary: SummaryItem[]
  paymentDay: number,
  onlyAnonWallet?: boolean
}

interface State {
  currentTab: number
  currentMonth: string
}

export default class ModalActivity extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      currentTab: 0,
      currentMonth: ''
    }
  }

  private summary: Record<SummaryType, {color: TokenType, translation: string}> = {
    grant: {
      color: 'earning',
      translation: this.props.onlyAnonWallet ? 'pointGrantClaimed' : 'tokenGrantClaimed'
    },
    ads: {
      color: 'earning',
      translation: 'earningsAds'
    },
    contribute: {
      color: 'contribute',
      translation: 'rewardsContribute'
    },
    monthly: {
      color: 'contribute',
      translation: 'recurringDonations'
    },
    tip: {
      color: 'contribute',
      translation: 'oneTimeDonation'
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

  getMonthlyDropDown = () => {
    const {
      id,
      onMonthChange,
      months
    } = this.props

    return (
      <ControlWrapper text={this.modalTitle}>
        <Select
          value={this.state.currentMonth}
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
    )
  }

  getTransactionTable = () => {
    const { transactionRows } = this.props

    return (
      <TableTransactions
        rows={transactionRows}
      >
        {getLocale('noActivity')}
      </TableTransactions>
    )
  }

  getMonthlyContributionTable = () => {
    const { activityRows, paymentDay } = this.props
    const rows = activityRows.filter(row => row.type === 'monthly')

    return (
      <>
        <TableActivity
          rows={rows}
        >
          {getLocale('noActivity')}
        </TableActivity>
        <PaymentMonthly>
          {getLocale('paymentMonthly', { day: paymentDay })}
        </PaymentMonthly>
      </>
    )
  }

  getAutoContributeTable = () => {
    const { activityRows, paymentDay } = this.props
    const rows = activityRows.filter(row => row.type === 'contribute')

    return (
      <>
        <TableActivity
          rows={rows}
        >
          {getLocale('noActivity')}
        </TableActivity>
        <PaymentMonthly>
          {getLocale('paymentMonthly', { day: paymentDay })}
        </PaymentMonthly>
      </>
    )
  }

  getOneTimeTips = () => {
    const { activityRows } = this.props
    const rows = activityRows.filter(row => row.type === 'tip')

    return (
      <>
        <TableActivity
          rows={rows}
          showDate={true}
        >
          {getLocale('noActivity')}
        </TableActivity>
      </>
    )
  }

  changeTab = (i: number) => {
    this.setState({
      currentTab: i
    })
  }

  generateTabs = () => {
    const tabs = [
      {
        title: getLocale('transactions'),
        content: this.getTransactionTable
      },
      {
        title: getLocale('monthlyContributions'),
        content: this.getMonthlyContributionTable
      },
      {
        title: getLocale('autoContribute'),
        content: this.getAutoContributeTable
      },
      {
        title: getLocale('oneTimeDonation'),
        content: this.getOneTimeTips
      }
    ]

    return (
      <>
        <Tabs>
          {
            tabs.map((tab, i) => {
              const isFirst = i === 0
              const selected = i === this.state.currentTab

              return (
                <Tab
                  key={`activity-tab-${i}`}
                  selected={selected}
                  isFirst={isFirst}
                  onClick={this.changeTab.bind(this, i)}
                >
                  {tab.title}
                </Tab>
              )
            })
          }
        </Tabs>
        <TabContent>
          {tabs[this.state.currentTab].content()}
        </TabContent>
      </>
    )
  }

  render () {
    const {
      id,
      onClose,
      months,
      onPrint
    } = this.props

    return (
      <Modal id={id} onClose={onClose}>
        <StyledWrapper>
          <StyledHeader>
            <StyledLeft>
              {
                months ? this.getMonthlyDropDown() : null
              }
            </StyledLeft>
            <StyledRight>
              <StyledIconWrap>
                {
                  onPrint
                  ? <StyledIcon onClick={onPrint}>
                    <StyledActionIcon>
                      <PrintIcon />
                    </StyledActionIcon>
                    <StyledIconText>{getLocale('print')}</StyledIconText>
                  </StyledIcon>
                  : null
                }
              </StyledIconWrap>
              {this.getSummaryBox()}
            </StyledRight>
          </StyledHeader>
          <StyledTables>
            {this.generateTabs()}
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
