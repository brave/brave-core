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

// #include "brave/third_party/flower/src/cc/flwr/include/client.h"
// #include "brave/third_party/flower/src/cc/flwr/include/serde.h"

using flower::ClientMessage;
using flower::ClientMessage_DisconnectRes;
using flower::ClientMessage_EvaluateRes;
using flower::ClientMessage_FitRes;
using flower::GetTasksRequest;
using flower::Reason;
using flower::ServerMessage;
using flower::ServerMessage_EvaluateIns;
using flower::ServerMessage_FitIns;
using flower::ServerMessage_ReconnectIns;

std::tuple<ClientMessage, int> _Reconnect(
    ServerMessage_ReconnectIns reconnect_msg);

ClientMessage _GetParameters(flower::Client* client);

ClientMessage _Fit(flower::Client* client, ServerMessage_FitIns fit_msg);

ClientMessage _Evaluate(flower::Client* client,
                        ServerMessage_EvaluateIns evaluate_msg);

std::tuple<ClientMessage, int, bool> HandleMessage(flower::Client* client,
                                                   ServerMessage server_msg);

GetTasksRequest GetTasksRequest();

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_MESSAGE_HANDLER_H_
