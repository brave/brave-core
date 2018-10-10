#include "base/command_line.h"
#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/process/kill.h"

namespace {

#if defined(OS_MACOSX)
// tor::switches::kTorExecutablePath
const char kTorExecutablePath[] = "tor-executable-path";

void CleanupTor() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kTorExecutablePath)) {
    class ProcessTreeFilter : public base::ProcessFilter {
      public:
        explicit ProcessTreeFilter(base::ProcessId parent_pid) {
          ancestor_pids_.insert(parent_pid);
        }
        bool Includes(const base::ProcessEntry& entry) const override {
          if (ancestor_pids_.find(entry.parent_pid()) != ancestor_pids_.end()) {
            ancestor_pids_.insert(entry.pid());
            return true;
          } else {
            return false;
          }
        }
      private:
        mutable std::set<base::ProcessId> ancestor_pids_;
    } process_tree_filter(base::Process::Current().Pid());

    base::FilePath path = base::CommandLine::ForCurrentProcess()
      ->GetSwitchValuePath(kTorExecutablePath);
    DCHECK(!path.empty());
    base::KillProcesses(path.value(), 0, &process_tree_filter);
  }
}
#endif // defined(OS_MACOSX)

}  // namespace

#include "../../../../../content/child/child_thread_impl.cc"
