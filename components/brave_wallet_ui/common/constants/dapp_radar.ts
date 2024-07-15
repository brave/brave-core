// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// constants
import { BraveWallet } from '../../constants/types'

export const dappRadarChainNamesToChainIdMapping: Record<string, string> = {
  'aleph-zero': '', // NOT SUPPORTED
  'algorand': '', // NOT SUPPORTED
  'aptos': '', // NOT SUPPORTED
  'arbitrum': BraveWallet.ARBITRUM_MAINNET_CHAIN_ID,
  'astar': BraveWallet.ASTAR_CHAIN_ID,
  'astar-zkevm': BraveWallet.ASTAR_ZK_EVM_CHAIN_ID,
  'aurora': BraveWallet.AURORA_MAINNET_CHAIN_ID,
  'avalanche': BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  'bahamut': BraveWallet.BAHAMUT_CHAIN_ID,
  'base': BraveWallet.BASE_MAINNET_CHAIN_ID,
  'blast': BraveWallet.BLAST_MAINNET_CHAIN_ID,
  'bnb-chain': BraveWallet.BNB_SMART_CHAIN_MAINNET_CHAIN_ID,
  'bttc': BraveWallet.BIT_TORRENT_CHAIN_MAINNET_CHAIN_ID,
  'cardano': '', // NOT SUPPORTED
  'celo': BraveWallet.CELO_MAINNET_CHAIN_ID,
  'chromia': '', // NOT SUPPORTED
  'core': BraveWallet.CORE_CHAIN_ID,
  'cronos': BraveWallet.CRONOS_MAINNET_CHAIN_ID,
  'cyber': BraveWallet.CYBER_MAINNET_CHAIN_ID,
  'defikingdoms': '', // NOT SUPPORTED
  'elysium': BraveWallet.ELYSIUM_MAINNET_CHAIN_ID,
  'eos': '', // NOT SUPPORTED
  'eosevm': BraveWallet.EOSEVM_NETWORK_CHAIN_ID,
  'ethereum': BraveWallet.MAINNET_CHAIN_ID,
  'fantom': BraveWallet.FANTOM_MAINNET_CHAIN_ID,
  'fio': '', // NOT SUPPORTED
  'flow': '', // NOT SUPPORTED
  'hedera': BraveWallet.HEDERA_MAINNET_CHAIN_ID,
  'hive': '', // NOT SUPPORTED
  'icp': '', // NOT SUPPORTED
  'immutablex': '', // NOT SUPPORTED
  'immutablezkevm': BraveWallet.IMMUTABLE_ZK_EVM_CHAIN_ID,
  'klaytn': BraveWallet.KLAYTN_MAINNET_CYPRESS_CHAIN_ID,
  'kroma': BraveWallet.KROMA_CHAIN_ID,
  'lightlink': BraveWallet.LIGHTLINK_PHOENIX_MAINNET_CHAIN_ID,
  'linea': BraveWallet.LINEA_CHAIN_ID,
  'mooi': '', // NOT SUPPORTED
  'moonbeam': BraveWallet.MOONBEAM_CHAIN_ID,
  'moonriver': BraveWallet.MOONRIVER_CHAIN_ID,
  'near': BraveWallet.NEAR_MAINNET_CHAIN_ID,
  'oasis': BraveWallet.OASIS_EMERALD_CHAIN_ID,
  'oasis-sapphire': BraveWallet.OASIS_SAPPHIRE_CHAIN_ID,
  'oasys-chain-verse': '', // NOT SUPPORTED
  'oasys-defi-verse': '', // NOT SUPPORTED
  'oasys-gesoten-verse': '', // NOT SUPPORTED
  'oasys-home-verse': '', // NOT SUPPORTED
  'oasys-mainnet': BraveWallet.OASYS_MAINNET_CHAIN_ID,
  'oasys-mch-verse': '', // NOT SUPPORTED
  'oasys-saakuru-verse': '', // NOT SUPPORTED
  'oasys-tcg-verse': '', // NOT SUPPORTED
  'oasys-yooldo-verse': '', // NOT SUPPORTED
  'ontology': BraveWallet.ONTOLOGY_MAINNET_CHAIN_ID,
  'opbnb': BraveWallet.OP_BNB_MAINNET_CHAIN_ID,
  'optimism': BraveWallet.OPTIMISM_MAINNET_CHAIN_ID,
  'other': '', // NOT SUPPORTED
  'polygon': BraveWallet.POLYGON_MAINNET_CHAIN_ID,
  'rangers': BraveWallet.RANGERS_PROTOCOL_MAINNET_CHAIN_ID,
  'ronin': BraveWallet.RONIN_CHAIN_ID,
  'shiden': BraveWallet.SHIDEN_CHAIN_ID,
  'skale-calypso': BraveWallet.SKALE_CALYPSO_HUB_CHAIN_ID,
  'skale-europa': BraveWallet.SKALE_EUROPA_HUB_CHAIN_ID,
  'skale-exorde': '', // NOT SUPPORTED
  'skale-nebula': BraveWallet.SKALE_NEBULA_HUB_CHAIN_ID,
  'skale-omnus': '', // NOT SUPPORTED
  'skale-razor': '', // NOT SUPPORTED
  'skale-strayshot': '', // NOT SUPPORTED
  'skale-titan': BraveWallet.SKALE_TITAN_HUB_CHAIN_ID,
  'solana': BraveWallet.SOLANA_MAINNET,
  'stacks': '', // NOT SUPPORTED
  'stargaze': '', // NOT SUPPORTED
  'steem': '', // NOT SUPPORTED
  'telos': '', // NOT SUPPORTED
  'telosevm': BraveWallet.TELOS_EVM_MAINNET_CHAIN_ID,
  'tezos': '', // NOT SUPPORTED
  'theta': BraveWallet.THETA_MAINNET_CHAIN_ID,
  'thundercore': BraveWallet.THUNDER_CORE_MAINNET_CHAIN_ID,
  'ton': '', // NOT SUPPORTED
  'tron': '', // NOT SUPPORTED
  'wax': '', // NOT SUPPORTED
  'wemix': BraveWallet.WEMIX_MAINNET_CHAIN_ID,
  'xai': BraveWallet.XAI_MAINNET_CHAIN_ID,
  'xrpl': '', // NOT SUPPORTED
  'zetachain': BraveWallet.ZETA_CHAIN_MAINNET_CHAIN_ID,
  'zilliqa': BraveWallet.ZILLIQA_EVM_CHAIN_ID,
  'zksync-era': BraveWallet.ZK_SYNC_ERA_CHAIN_ID
}
