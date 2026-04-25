cfg_sync!(
    mod sync_impl;
    pub use sync_impl::FileExt;
);

cfg_async!(
    pub(crate) mod async_impl;
);
