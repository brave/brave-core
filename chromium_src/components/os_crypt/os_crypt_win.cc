#include "base/command_line.h"
#include "brave/common/brave_switches.h"

// switches::kDisableEncryptionWin
const char kDisableEncryptionWin[] = "disable-encryption-win";

namespace {
bool IsEncryptionDisabled(const std::string& input_text, std::string* output_text) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kDisableEncryptionWin)) {
    *output_text = input_text;
    return true;
  }
  return false;
}

}  // namespace
#include "../../../../components/os_crypt/os_crypt_win.cc"
