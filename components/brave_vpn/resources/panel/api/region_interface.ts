export interface Region {
  continent: string
  name: string
  namePretty: string
}
export interface RegionState {
  all?: Region[]
  current?: Region
  hasError: boolean
}
