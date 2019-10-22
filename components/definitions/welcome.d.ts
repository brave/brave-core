declare namespace Welcome {
  export interface ApplicationState {
    welcomeData: State | undefined
  }

  export interface BrowserTheme {
    name: string
    index: string
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
    browserProfiles: Array<BrowserProfile>
    browserThemes: Array<BrowserTheme>
    walletCreated: boolean
    walletCreating: boolean
    walletCreateFailed: boolean
  }

  export const enum WalletResult {
    LEDGER_OK = 0,
    LEDGER_ERROR = 1,
    NO_PUBLISHER_STATE = 2,
    NO_LEDGER_STATE = 3,
    INVALID_PUBLISHER_STATE = 4,
    INVALID_LEDGER_STATE = 5,
    CAPTCHA_FAILED = 6,
    NO_PUBLISHER_LIST = 7,
    TOO_MANY_RESULTS = 8,
    NOT_FOUND = 9,
    REGISTRATION_VERIFICATION_FAILED = 10,
    BAD_REGISTRATION_RESPONSE = 11,
    WALLET_CREATED = 12,
    GRANT_NOT_FOUND = 13,
    WALLET_CORRUPT = 17
  }
}
