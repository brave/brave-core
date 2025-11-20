#ifndef BRAVE_INSTALLER_SETUP_SETUP_UTIL_H_
#define BRAVE_INSTALLER_SETUP_SETUP_UTIL_H_

namespace base {
class FilePath;
class Version;
}  // namespace base

namespace installer {

class InstallationState;
class InstallerState;

// Returns the uncompressed archive of the installed version that serves as the
// source for patching.  If |desired_version| is valid, only the path to that
// version will be returned, or empty if it doesn't exist.
//
// This function used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
base::FilePath FindArchiveToPatch(const InstallationState& original_state,
                                  const InstallerState& installer_state,
                                  const base::Version& desired_version);

}  // namespace installer

#endif  // BRAVE_INSTALLER_SETUP_SETUP_UTIL_H_
