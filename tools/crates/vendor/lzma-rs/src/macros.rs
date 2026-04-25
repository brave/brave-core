/// Log trace message (feature: enabled).
#[cfg(feature = "enable_logging")]
macro_rules! lzma_trace {
    ($($arg:tt)+) => {
        log::trace!($($arg)+);
    }
}

/// Log debug message (feature: enabled).
#[cfg(feature = "enable_logging")]
macro_rules! lzma_debug {
    ($($arg:tt)+) => {
        log::debug!($($arg)+);
    }
}

/// Log info message (feature: enabled).
#[cfg(feature = "enable_logging")]
macro_rules! lzma_info {
    ($($arg:tt)+) => {
        log::info!($($arg)+);
    }
}

/// Log trace message (feature: disabled).
#[cfg(not(feature = "enable_logging"))]
macro_rules! lzma_trace {
    ($($arg:tt)+) => {};
}

/// Log debug message (feature: disabled).
#[cfg(not(feature = "enable_logging"))]
macro_rules! lzma_debug {
    ($($arg:tt)+) => {};
}

/// Log info message (feature: disabled).
#[cfg(not(feature = "enable_logging"))]
macro_rules! lzma_info {
    ($($arg:tt)+) => {};
}
