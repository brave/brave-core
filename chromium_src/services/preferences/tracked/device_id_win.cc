#include "base/command_line.h"
#include "brave/common/brave_switches.h"

// switches::kDisableMachineId
const char kDisableMachineId[] = "disable-machine-id";

namespace {
bool IsMachineIdDisabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(kDisableMachineId);
}

}  // namespace
#include "../../../../../services/preferences/tracked/device_id_win.cc"
