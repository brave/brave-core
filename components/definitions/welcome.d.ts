declare namespace Welcome {
  export interface ApplicationState {
    welcomeData: State | undefined
  }

  export interface BrowserProfile {
    autofillFormData: boolean,
    cookies: boolean,
    favorites: boolean,
    history: boolean,
    index: number,
    ledger: boolean,
    name: string,
    passwords: boolean,
    search: boolean,
    stats: boolean,
    windows: boolean
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
    defaults: Array<SearchEngineEntry>
    extensions: Array<any>
    others: Array<any>
  }

  export interface State {
    searchProviders: Array<SearchEngineEntry>
    browserProfiles: Array<BrowserProfile>,
    hideSearchOnboarding: boolean
  }
}
