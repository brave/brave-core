#include "base/command_line.h"
#include "base/process/launch.h"

namespace {
void BraveLaunchOption(base::CommandLine* cmd_line,
                       base::LaunchOptions *options) {
  // tor::swtiches::kTorExecutablePath
  if (cmd_line->HasSwitch("tor-executable-path"))
    options->start_hidden = true;
}

}  // namespace
#include "../../../../../../../services/service_manager/sandbox/win/sandbox_win.cc"
