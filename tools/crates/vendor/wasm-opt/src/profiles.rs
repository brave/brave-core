use crate::api::{OptimizationOptions, OptimizeLevel, ShrinkLevel};

#[derive(Eq, PartialEq, Debug)]
pub struct Profile {
    optimize_level: OptimizeLevel,
    shrink_level: ShrinkLevel,
    add_default_passes: bool,
}

impl Profile {
    pub fn optimize_for_size() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level2,
            shrink_level: ShrinkLevel::Level1,
            add_default_passes: true,
        }
    }

    pub fn optimize_for_size_aggressively() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level2,
            shrink_level: ShrinkLevel::Level2,
            add_default_passes: true,
        }
    }

    pub fn opt_level_0() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level0,
            shrink_level: ShrinkLevel::Level0,
            add_default_passes: false,
        }
    }

    pub fn opt_level_1() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level1,
            shrink_level: ShrinkLevel::Level0,
            add_default_passes: true,
        }
    }

    pub fn opt_level_2() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level2,
            shrink_level: ShrinkLevel::Level0,
            add_default_passes: true,
        }
    }

    pub fn opt_level_3() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level3,
            shrink_level: ShrinkLevel::Level0,
            add_default_passes: true,
        }
    }

    pub fn opt_level_4() -> Profile {
        Profile {
            optimize_level: OptimizeLevel::Level4,
            shrink_level: ShrinkLevel::Level0,
            add_default_passes: true,
        }
    }

    pub fn into_opts(self) -> OptimizationOptions {
        let mut opts = OptimizationOptions::new_empty();
        self.apply_to_opts(&mut opts);
        opts
    }

    pub fn apply_to_opts(self, opts: &mut OptimizationOptions) {
        opts.passopts.optimize_level = self.optimize_level;
        opts.passopts.shrink_level = self.shrink_level;
        opts.passes.add_default_passes = self.add_default_passes;
    }
}
