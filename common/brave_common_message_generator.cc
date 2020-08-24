/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Get basic type definitions.
#define IPC_MESSAGE_IMPL

#include "brave/common/brave_common_message_generator.h"

// Generate constructors.
#include "ipc/struct_constructor_macros.h"
#include "brave/common/brave_common_message_generator.h"

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#include "brave/common/brave_common_message_generator.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#include "brave/common/brave_common_message_generator.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#include "brave/common/brave_common_message_generator.h"
}  // namespace IPC

