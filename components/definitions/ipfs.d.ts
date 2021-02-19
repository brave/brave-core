declare namespace IPFS {
  export interface ApplicationState {
    ipfsData: State | undefined
  }

  export interface State {
    connectedPeers: {
      peerCount: number
    }
    addressesConfig: AddressesConfig
    daemonStatus: DaemonStatus
    repoStats: RepoStats,
    nodeInfo: NodeInfo,
    garbageCollectionStatus: GarbageCollectionStatus,
  }

  export interface AddressesConfig {
    api: string
    gateway: string
    swarm: string[]
  }

  export interface DaemonStatus {
    installed: bool
    launched: bool
    restarting: bool
    installing: bool
  }

  export interface GarbageCollectionStatus {
    success: bool
    error: string
    started: bool
  }

  export interface RepoStats {
    objects: number
    size: number
    storage: number
    path: string
    version: string
  }

  export interface NodeInfo {
    id: string
    version: string
  }

}
