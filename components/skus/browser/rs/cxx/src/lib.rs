mod errors;
mod httpclient;
mod log;
mod storage;

use std::cell::RefCell;
use std::fmt;
use std::rc::Rc;

use cxx::{type_id, ExternType, UniquePtr};
use futures::executor::LocalPool;
use futures::task::LocalSpawnExt;

use tracing::debug;

pub use skus;

use crate::httpclient::{HttpRoundtripContext, WakeupContext};

pub struct NativeClientContext {
    environment: skus::Environment,
    ctx: UniquePtr<ffi::SkusSdkContext>,
}

#[derive(Clone)]
pub struct NativeClient {
    is_shutdown: Rc<RefCell<bool>>,
    pool: Rc<RefCell<LocalPool>>,
    ctx: Rc<RefCell<NativeClientContext>>,
}

impl fmt::Debug for NativeClient {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("NativeClient").finish()
    }
}

impl NativeClient {
    fn try_run_until_stalled(&self) {
        if *self.is_shutdown.borrow() {
            debug!("sdk is shutdown, exiting");
            return;
        }
        if let Ok(mut pool) = self.pool.try_borrow_mut() {
            pool.run_until_stalled();
        }
    }
}

#[allow(unused)]
#[cxx::bridge(namespace = skus)]
mod ffi {
    #[derive(Debug)]
    pub enum TracingLevel {
        /// The "trace" level.
        ///
        /// Designates very low priority, often extremely verbose, information.
        Trace = 0,
        /// The "debug" level.
        ///
        /// Designates lower priority information.
        Debug = 1,
        /// The "info" level.
        ///
        /// Designates useful information.
        Info = 2,
        /// The "warn" level.
        ///
        /// Designates hazardous situations.
        Warn = 3,
        /// The "error" level.
        ///
        /// Designates very serious errors.
        Error = 4,
    }

    #[derive(Debug)]
    pub enum SkusResult {
        Ok,
        RequestFailed,
        InternalServer,
        BadRequest,
        UnhandledStatus,
        RetryLater,
        NotFound,
        SerializationFailed,
        InvalidResponse,
        InvalidProof,
        QueryError,
        OutOfCredentials,
        StorageWriteFailed,
        StorageReadFailed,
        OrderUnpaid,
        UnhandledVariant,
        OrderLocationMismatch,
        ItemCredentialsMissing,
        ItemCredentialsExpired,
        InvalidMerchantOrSku,
        UnknownError,
        BorrowFailed,
        FutureCancelled,
    }

    pub struct HttpRequest {
        pub url: String,
        pub method: String,
        pub headers: Vec<String>,
        pub body: Vec<u8>,
    }
    #[derive(Debug)]
    pub struct HttpResponse<'a> {
        pub result: SkusResult,
        pub return_code: u16,
        pub headers: &'a CxxVector<CxxString>,
        pub body: &'a CxxVector<u8>,
    }

    extern "Rust" {
        type HttpRoundtripContext;
        type WakeupContext;

        type CppSDK;
        fn initialize_sdk(ctx: UniquePtr<SkusSdkContext>, env: String) -> Box<CppSDK>;
        fn shutdown(self: &CppSDK);
        fn refresh_order(
            self: &CppSDK,
            callback: RefreshOrderCallback,
            callback_state: UniquePtr<RefreshOrderCallbackState>,
            order_id: String,
        );
        fn fetch_order_credentials(
            self: &CppSDK,
            callback: FetchOrderCredentialsCallback,
            callback_state: UniquePtr<FetchOrderCredentialsCallbackState>,
            order_id: String,
        );
        fn prepare_credentials_presentation(
            self: &CppSDK,
            callback: PrepareCredentialsPresentationCallback,
            callback_state: UniquePtr<PrepareCredentialsPresentationCallbackState>,
            domain: String,
            path: String,
        );
        fn credential_summary(
            self: &CppSDK,
            callback: CredentialSummaryCallback,
            callback_state: UniquePtr<CredentialSummaryCallbackState>,
            domain: String,
        );
    }

    unsafe extern "C++" {
        include!("brave/components/skus/browser/rs/cxx/src/shim.h");

        type SkusSdkContext;
        type SkusSdkFetcher;

        fn shim_logMessage(file: &str, line: u32, level: TracingLevel, message: &str);

        fn shim_executeRequest(
            ctx: &SkusSdkContext,
            req: &HttpRequest,
            done: fn(Box<HttpRoundtripContext>, resp: HttpResponse),
            rt_ctx: Box<HttpRoundtripContext>,
        ) -> UniquePtr<SkusSdkFetcher>;

        fn shim_scheduleWakeup(
            delay_ms: u64,
            done: fn(Box<WakeupContext>),
            ctx: Box<WakeupContext>,
        );

        fn shim_purge(ctx: Pin<&mut SkusSdkContext>);
        fn shim_set(ctx: Pin<&mut SkusSdkContext>, key: &str, value: &str);
        fn shim_get(ctx: Pin<&mut SkusSdkContext>, key: &str) -> String;

        type RefreshOrderCallbackState;
        type RefreshOrderCallback = crate::RefreshOrderCallback;
        type FetchOrderCredentialsCallbackState;
        type FetchOrderCredentialsCallback = crate::FetchOrderCredentialsCallback;
        type PrepareCredentialsPresentationCallbackState;
        type PrepareCredentialsPresentationCallback = crate::PrepareCredentialsPresentationCallback;
        type CredentialSummaryCallbackState;
        type CredentialSummaryCallback = crate::CredentialSummaryCallback;
    }
}

pub struct CppSDK {
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
}

fn initialize_sdk(ctx: UniquePtr<ffi::SkusSdkContext>, env: String) -> Box<CppSDK> {
    tracing_subscriber::fmt()
        .event_format(log::CppFormatter::new())
        .with_max_level(tracing::Level::TRACE)
        .init();

    let env = env
        .parse::<skus::Environment>()
        .unwrap_or(skus::Environment::Local);

    let sdk = skus::sdk::SDK::new(
        NativeClient {
            is_shutdown: Rc::new(RefCell::new(false)),
            pool: Rc::new(RefCell::new(LocalPool::new())),
            ctx: Rc::new(RefCell::new(NativeClientContext {
                environment: env.clone(),
                ctx,
            })),
        },
        env,
        None,
        None,
    );
    let sdk = Rc::new(sdk);
    let mut spawner = sdk.client.pool.borrow_mut().spawner();
    {
        let sdk = sdk.clone();

        if spawner
            .spawn_local(async move {
                sdk.initialize().await;
            })
            .is_err()
        {
            debug!("pool is shutdown");
        }
    }

    sdk.client.pool.borrow_mut().run_until_stalled();

    Box::new(CppSDK { sdk })
}

impl CppSDK {
    fn shutdown(&self) {
        // drop any existing futures
        drop(self.sdk.client.pool.take());
        // ensure lingering callbacks passed to c++ are short circuited
        *self.sdk.client.is_shutdown.borrow_mut() = true;
    }

    fn refresh_order(
        &self,
        callback: RefreshOrderCallback,
        callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
        order_id: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        if spawner
            .spawn_local(refresh_order_task(
                self.sdk.clone(),
                callback,
                callback_state,
                order_id,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }

    fn fetch_order_credentials(
        &self,
        callback: FetchOrderCredentialsCallback,
        callback_state: UniquePtr<ffi::FetchOrderCredentialsCallbackState>,
        order_id: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        if spawner
            .spawn_local(fetch_order_credentials_task(
                self.sdk.clone(),
                callback,
                callback_state,
                order_id,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }

    fn prepare_credentials_presentation(
        &self,
        callback: PrepareCredentialsPresentationCallback,
        callback_state: UniquePtr<ffi::PrepareCredentialsPresentationCallbackState>,
        domain: String,
        path: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        if spawner
            .spawn_local(prepare_credentials_presentation_task(
                self.sdk.clone(),
                callback,
                callback_state,
                domain,
                path,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }

    fn credential_summary(
        &self,
        callback: CredentialSummaryCallback,
        callback_state: UniquePtr<ffi::CredentialSummaryCallbackState>,
        domain: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        if spawner
            .spawn_local(credential_summary_task(
                self.sdk.clone(),
                callback,
                callback_state,
                domain,
            ))
            .is_err()
        {
            debug!("pool is shutdown");
        }

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }
}

#[repr(transparent)]
pub struct RefreshOrderCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::RefreshOrderCallbackState,
        result: ffi::SkusResult,
        order: &str,
    ),
);

unsafe impl ExternType for RefreshOrderCallback {
    type Id = type_id!("skus::RefreshOrderCallback");
    type Kind = cxx::kind::Trivial;
}

async fn refresh_order_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: RefreshOrderCallback,
    callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
    order_id: String,
) {
    match sdk
        .refresh_order(&order_id)
        .await
        .and_then(|order| serde_json::to_string(&order).map_err(|e| e.into()))
        .map_err(|e| e.into())
    {
        Ok(order) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, &order),
        Err(e) => callback.0(callback_state.into_raw(), e, ""),
    }
}

#[repr(transparent)]
pub struct FetchOrderCredentialsCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::FetchOrderCredentialsCallbackState,
        result: ffi::SkusResult,
    ),
);

unsafe impl ExternType for FetchOrderCredentialsCallback {
    type Id = type_id!("skus::FetchOrderCredentialsCallback");
    type Kind = cxx::kind::Trivial;
}

async fn fetch_order_credentials_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: FetchOrderCredentialsCallback,
    callback_state: UniquePtr<ffi::FetchOrderCredentialsCallbackState>,
    order_id: String,
) {
    match sdk
        .fetch_order_credentials(&order_id)
        .await
        .map_err(|e| e.into())
    {
        Ok(_) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok),
        Err(e) => callback.0(callback_state.into_raw(), e),
    }
}

#[repr(transparent)]
pub struct PrepareCredentialsPresentationCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::PrepareCredentialsPresentationCallbackState,
        result: ffi::SkusResult,
        presentation: &str,
    ),
);

unsafe impl ExternType for PrepareCredentialsPresentationCallback {
    type Id = type_id!("skus::PrepareCredentialsPresentationCallback");
    type Kind = cxx::kind::Trivial;
}

async fn prepare_credentials_presentation_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: PrepareCredentialsPresentationCallback,
    callback_state: UniquePtr<ffi::PrepareCredentialsPresentationCallbackState>,
    domain: String,
    path: String,
) {
    match sdk
        .prepare_credentials_presentation(&domain, &path)
        .await
        .map_err(|e| e.into())
    {
        Ok(Some(presentation)) => callback.0(
            callback_state.into_raw(),
            ffi::SkusResult::Ok,
            &presentation,
        ),
        Ok(None) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, ""),
        Err(e) => callback.0(callback_state.into_raw(), e, ""),
    }
}

#[repr(transparent)]
pub struct CredentialSummaryCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::CredentialSummaryCallbackState,
        result: ffi::SkusResult,
        summary: &str,
    ),
);

unsafe impl ExternType for CredentialSummaryCallback {
    type Id = type_id!("skus::CredentialSummaryCallback");
    type Kind = cxx::kind::Trivial;
}

async fn credential_summary_task(
    sdk: Rc<skus::sdk::SDK<NativeClient>>,
    callback: CredentialSummaryCallback,
    callback_state: UniquePtr<ffi::CredentialSummaryCallbackState>,
    domain: String,
) {
    match sdk
        .matching_credential_summary(&domain)
        .await
        .and_then(|summary| {
            summary
                .map(|summary| serde_json::to_string(&summary).map_err(|e| e.into()))
                .transpose()
        })
        .map_err(|e| e.into())
    {
        Ok(Some(summary)) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, &summary),
        Ok(None) => callback.0(callback_state.into_raw(), ffi::SkusResult::Ok, ""),
        Err(e) => callback.0(callback_state.into_raw(), e, ""),
    }
}
