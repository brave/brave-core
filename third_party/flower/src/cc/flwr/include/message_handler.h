/*************************************************************************************************
 * Copyright (c) 2022 The Flower Authors.
 *
 * @file message_handler.h
 *
 * @brief Handle server messages by calling appropriate client methods
 *
 * @author Lekang Jiang
 *
 * @version 1.0
 *
 * @date 04/09/2021
 *
 *************************************************************************************************/

#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_MESSAGE_HANDLER_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_MESSAGE_HANDLER_H_

#include <tuple>

#include "brave/third_party/flower/src/cc/flwr/include/client.h"
#include "brave/third_party/flower/src/cc/flwr/include/serde.h"

using flower::transport::ClientMessage;
using flower::transport::ClientMessage_Disconnect;
using flower::transport::ClientMessage_EvaluateRes;
using flower::transport::ClientMessage_FitRes;
using flower::transport::Reason;
using flower::transport::ServerMessage;
using flower::transport::ServerMessage_EvaluateIns;
using flower::transport::ServerMessage_FitIns;
using flower::transport::ServerMessage_Reconnect;

std::tuple<ClientMessage, int> _Reconnect(
    ServerMessage_Reconnect reconnect_msg);

ClientMessage _GetParameters(flwr::Client* client);

ClientMessage _Fit(flwr::Client* client, ServerMessage_FitIns fit_msg);

ClientMessage _Evaluate(flwr::Client* client,
                        ServerMessage_EvaluateIns evaluate_msg);

std::tuple<ClientMessage, int, bool> HandleMessage(flwr::Client* client,
                                            ServerMessage server_msg);

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_MESSAGE_HANDLER_H_
