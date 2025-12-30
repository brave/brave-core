use crate::{UIImageResizingMode, TARGET_ABI_USES_IOS_VALUES};

#[allow(non_upper_case_globals)]
#[allow(clippy::bool_to_int_with_if)]
impl UIImageResizingMode {
    #[doc(alias = "UIImageResizingModeStretch")]
    pub const Stretch: Self = Self(if TARGET_ABI_USES_IOS_VALUES { 0 } else { 1 });
    #[doc(alias = "UIImageResizingModeTile")]
    pub const Tile: Self = Self(if TARGET_ABI_USES_IOS_VALUES { 1 } else { 0 });
}
