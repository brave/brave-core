// ctl_flags.rs

use super::consts::*;

// Represents control flags of a sysctl
bitflags! {
    pub struct CtlFlags : libc::c_uint {
        /// Allow reads of variable
        const RD = CTLFLAG_RD;

        /// Allow writes to the variable
        const WR = CTLFLAG_WR;

        const RW = Self::RD.bits() | Self::WR.bits();

        /// This sysctl is not active yet
        const DORMANT = CTLFLAG_DORMANT;

        /// All users can set this var
        const ANYBODY = CTLFLAG_ANYBODY;

        /// Permit set only if securelevel<=0
        const SECURE = CTLFLAG_SECURE;

        /// Prisoned roots can fiddle
        const PRISON = CTLFLAG_PRISON;

        /// Dynamic oid - can be freed
        const DYN = CTLFLAG_DYN;

        /// Skip this sysctl when listing
        const SKIP = CTLFLAG_DORMANT;

        /// Secure level
        const SECURE_MASK = 0x00F00000;

        /// Default value is loaded from getenv()
        const TUN = CTLFLAG_TUN;

        /// Readable tunable
        const RDTUN = Self::RD.bits() | Self::TUN.bits();

        /// Readable and writeable tunable
        const RWTUN = Self::RW.bits() | Self::TUN.bits();

        /// Handler is MP safe
        const MPSAFE = CTLFLAG_MPSAFE;

        /// Prisons with vnet can fiddle
        const VNET = CTLFLAG_VNET;

        /// Oid is being removed
        const DYING = CTLFLAG_DYING;

        /// Can be read in capability mode
        const CAPRD = CTLFLAG_CAPRD;

        /// Can be written in capability mode
        const CAPWR = CTLFLAG_CAPWR;

        /// Statistics; not a tuneable
        const STATS = CTLFLAG_STATS;

        /// Don't fetch tunable from getenv()
        const NOFETCH = CTLFLAG_NOFETCH;

        /// Can be read and written in capability mode
        const CAPRW = Self::CAPRD.bits() | Self::CAPWR.bits();
    }
}

#[cfg(test)]
mod tests {
    use crate::Sysctl;

    #[test]
    fn ctl_flags() {
        // This sysctl should be read-only.
        #[cfg(any(target_os = "freebsd", target_os = "macos", target_os = "ios"))]
        let ctl: crate::Ctl = crate::Ctl::new("kern.ostype").unwrap();
        #[cfg(any(target_os = "android", target_os = "linux"))]
        let ctl: crate::Ctl = crate::Ctl::new("kernel.ostype").unwrap();

        let flags: crate::CtlFlags = ctl.flags().unwrap();

        assert_eq!(flags.bits() & crate::CTLFLAG_RD, crate::CTLFLAG_RD);
        assert_eq!(flags.bits() & crate::CTLFLAG_WR, 0);
    }
}
