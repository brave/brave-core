#ifndef BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_SERDE2_H_
#define BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_SERDE2_H_

#include <string>

#include "brave/third_party/flower/src/proto/flwr/proto/fleet.grpc.pb.h"

using TaskRequestMessage = flower::GetTasksRequest;

TaskRequestMessage GetTasksRequestMessage() {
  TaskRequestMessage trm;
  trm.set_id(111);

  return trm;
}

#endif  // BRAVE_THIRD_PARTY_FLOWER_SRC_CC_FLWR_INCLUDE_SERDE2_H_
