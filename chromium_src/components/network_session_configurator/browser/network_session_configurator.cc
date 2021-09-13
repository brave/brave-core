/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define ParseCommandLineAndFieldTrials \
  ParseCommandLineAndFieldTrials_ChromiumImpl
#include "../../../../../components/network_session_configurator/browser/network_session_configurator.cc"
#undef ParseCommandLineAndFieldTrials

namespace network_session_configurator {

// Never send QUIC user agent.
void ParseCommandLineAndFieldTrials(const base::CommandLine& command_line,
                                    bool is_quic_force_disabled,
                                    const std::string& quic_user_agent_id,
                                    net::HttpNetworkSessionParams* params,
                                    net::QuicParams* quic_params) {
  ParseCommandLineAndFieldTrials_ChromiumImpl(
      command_line,
      is_quic_force_disabled,
      "" /* quic_user_agent_id */,
      params,
      quic_params);
}

}  // namespace network_session_configurator
