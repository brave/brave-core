#ifndef BRAVE_COMMON_BRAVE_DEBUGGER_MESSAGES_H_
#define BRAVE_COMMON_BRAVE_DEBUGGER_MESSAGES_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "ipc/ipc_message_macros.h"
#include "url/origin.h"

// IPC messages for Brave debugger API communication

#define IPC_MESSAGE_START BraveDebuggerMsgStart

// Browser -> Renderer: Response to debugger commands
IPC_MESSAGE_ROUTED3(BraveDebuggerMsg_AttachResponse,
                   int /* request_id */,
                   bool /* success */,
                   std::string /* error_message */)

IPC_MESSAGE_ROUTED4(BraveDebuggerMsg_CommandResponse,
                   int /* request_id */,
                   bool /* success */,
                   base::Value::Dict /* result */,
                   std::string /* error_message */)

IPC_MESSAGE_ROUTED2(BraveDebuggerMsg_TargetsResponse,
                   int /* request_id */,
                   std::vector<base::Value::Dict> /* targets */)

// Renderer -> Browser: Debugger API calls
IPC_MESSAGE_ROUTED4(BraveDebuggerHostMsg_Attach,
                   int /* request_id */,
                   std::string /* target_id */,
                   std::string /* version */,
                   url::Origin /* origin */)

IPC_MESSAGE_ROUTED3(BraveDebuggerHostMsg_Detach,
                   int /* request_id */,
                   std::string /* target_id */,
                   url::Origin /* origin */)

IPC_MESSAGE_ROUTED5(BraveDebuggerHostMsg_SendCommand,
                   int /* request_id */,
                   std::string /* target_id */,
                   std::string /* method */,
                   base::Value::Dict /* params */,
                   url::Origin /* origin */)

IPC_MESSAGE_ROUTED2(BraveDebuggerHostMsg_GetTargets,
                   int /* request_id */,
                   url::Origin /* origin */)

#endif  // BRAVE_COMMON_BRAVE_DEBUGGER_MESSAGES_H_