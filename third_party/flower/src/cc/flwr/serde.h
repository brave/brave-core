/***********************************************************************************************************
 * Copyright (c) 2022 The Flower Authors.
 *
 * @file serde.h
 *
 * @brief ProtoBuf serialization and deserialization
 *
 * @author Lekang Jiang
 *
 * @version 1.0
 *
 * @date 03/09/2021
 *
 * ********************************************************************************************************/

#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_SERDE_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_SERDE_H_

#include <string>

#include "brave/third_party/flower/src/proto/flwr/proto/transport.pb.h"
#include "brave/third_party/flower/src/cc/flwr/include/typing.h"

using flower::ClientMessage;
using flower::ServerMessage;
using MessageParameters = flower::Parameters;
using flower::Reason;
using ProtoScalar = flower::Scalar;
using flower::ClientMessage_EvaluateRes;
using flower::ClientMessage_FitRes;
using flower::ClientMessage_GetParametersRes;
using flower::ServerMessage_EvaluateIns;
using flower::ServerMessage_FitIns;

/**
 * Serialize client parameters to protobuf parameters message
 */
MessageParameters parameters_to_proto(flower::Parameters parameters);

/**
 * Deserialize client protobuf parameters message to client parameters
 */
flower::Parameters parameters_from_proto(MessageParameters msg);

/**
 * Serialize client scalar type to protobuf scalar type
 */
ProtoScalar scalar_to_proto(flower::Scalar scalar_msg);

/**
 * Deserialize protobuf scalar type to client scalar type
 */
flower::Scalar scalar_from_proto(ProtoScalar scalar_msg);

/**
 * Serialize client metrics type to protobuf metrics type
 * "Any" is used in Python, this part might be changed if needed
 */
google::protobuf::Map<std::string, ProtoScalar> metrics_to_proto(
    flower::Metrics metrics);

/**
 * Deserialize protobuf metrics type to client metrics type
 * "Any" is used in Python, this part might be changed if needed
 */
flower::Metrics metrics_from_proto(
    google::protobuf::Map<std::string, ProtoScalar> proto);

/**
 * Serialize client ParametersRes type to protobuf ParametersRes type
 */
ClientMessage_GetParametersRes parameters_res_to_proto(
    flower::ParametersRes res);

/**
 * Deserialize protobuf FitIns type to client FitIns type
 */
flower::FitIns fit_ins_from_proto(ServerMessage_FitIns msg);

/**
 * Serialize client FitRes type to protobuf FitRes type
 */
ClientMessage_FitRes fit_res_to_proto(flower::FitRes res);

/**
 * Deserialize protobuf EvaluateIns type to client EvaluateIns type
 */
flower::EvaluateIns evaluate_ins_from_proto(ServerMessage_EvaluateIns msg);

/**
 * Serialize client EvaluateRes type to protobuf EvaluateRes type
 */
ClientMessage_EvaluateRes evaluate_res_to_proto(flower::EvaluateRes res);

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_SERDE_H_
