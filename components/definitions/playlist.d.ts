declare namespace Playlist {
  export interface ApplicationState {
    playlistData: State | undefined
  }

  export interface State {
    lists: []
  }
}
