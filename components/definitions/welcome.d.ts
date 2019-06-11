declare namespace Welcome {
  export interface ApplicationState {
    welcomeData: State | undefined
  }

  export interface SearchEngineEntry {
    canBeDefault: boolean,
    canBeEdited: boolean,
    canBeRemoved: boolean,
    default: boolean,
    displayName: string,
    iconURL: string,
    id: number,
    isOmniboxExtension: boolean,
    keyword: string,
    modelIndex: number,
    name: string,
    url: string,
    urlLocked: boolean
  }

  export interface SearchEngineListResponse {
    defaults: Array<Welcome.SearchEngineEntry>
    extensions: Array<any>
    others: Array<any>
  }

  export interface State {
    searchProviders: Array<SearchEngineEntry>
  }
}
