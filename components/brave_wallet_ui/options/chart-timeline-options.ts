import { ChartTimelineObjectType } from '../constants/types'
import locale from '../constants/locale'

export const ChartTimelineOptions: ChartTimelineObjectType[] = [
  {
    name: locale.chartLive,
    id: '5MIN'
  },
  {
    name: locale.chartOneDay,
    id: '24HRS'
  },
  {
    name: locale.chartOneWeek,
    id: '7Day'
  },
  {
    name: locale.chartOneMonth,
    id: '1Month'
  },
  {
    name: locale.chartThreeMonths,
    id: '3Months'
  },
  {
    name: locale.chartOneYear,
    id: '1Year'
  },
  {
    name: locale.chartAllTime,
    id: 'AllTime'
  }
]
