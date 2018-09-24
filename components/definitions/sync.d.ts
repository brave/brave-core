declare namespace Sync {
  export interface ApplicationState {
    syncData: State | undefined
  }

  export interface State {
    something: string
    // TODO
  }
}
