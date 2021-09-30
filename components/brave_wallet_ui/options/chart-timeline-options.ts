import { ChartTimelineObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const ChartTimelineOptions = (): ChartTimelineObjectType[] => [
  {
    name: getLocale('braveWalletChartLive'),
    id: 0
  },
  {
    name: getLocale('braveWalletChartOneDay'),
    id: 1
  },
  {
    name: getLocale('braveWalletChartOneWeek'),
    id: 2
  },
  {
    name: getLocale('braveWalletChartOneMonth'),
    id: 3
  },
  {
    name: getLocale('braveWalletChartThreeMonths'),
    id: 4
  },
  {
    name: getLocale('braveWalletChartOneYear'),
    id: 5
  },
  {
    name: getLocale('braveWalletChartAllTime'),
    id: 6
  }
]
