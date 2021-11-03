mod errors;
mod httpclient;
mod storage;

use std::cell::RefCell;
use std::error::Error;
use std::fmt;
use std::rc::Rc;

use cxx::{type_id, ExternType, UniquePtr};
use futures::executor::LocalPool;
use futures::task::LocalSpawnExt;

pub use brave_rewards;

use crate::httpclient::{HttpRoundtripContext, WakeupContext};

pub struct NativeClientContext(UniquePtr<ffi::SkusSdkContext>);

#[derive(Clone)]
pub struct NativeClient {
    pool: Rc<RefCell<LocalPool>>,
    ctx: Rc<RefCell<NativeClientContext>>,
}

impl fmt::Debug for NativeClient {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("NativeClient").finish()
    }
}

#[allow(unused)]
#[cxx::bridge(namespace = brave_rewards)]
mod ffi {
    #[derive(Debug)]
    pub enum RewardsResult {
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
    }

    pub struct HttpRequest {
        pub url: String,
        pub method: String,
        pub headers: Vec<String>,
        pub body: Vec<u8>,
    }
    #[derive(Debug)]
    pub struct HttpResponse<'a> {
        pub result: RewardsResult,
        pub return_code: u16,
        pub headers: &'a CxxVector<CxxString>,
        pub body: &'a CxxVector<u8>,
    }

    extern "Rust" {
        type HttpRoundtripContext;
        type WakeupContext;

        type CppSDK;
        fn initialize_sdk(ctx: UniquePtr<SkusSdkContext>, env: String) -> Box<CppSDK>;
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
        include!("shim.h");

        type SkusSdkContext;
        type SkusSdkFetcher;

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
    sdk: Rc<brave_rewards::sdk::SDK<NativeClient>>,
}

fn initialize_sdk(ctx: UniquePtr<ffi::SkusSdkContext>, env: String) -> Box<CppSDK> {
    // FIXME replace with ffi logging
    tracing_subscriber::fmt::init();

    // FIXME
    let sdk = brave_rewards::sdk::SDK::new(
        NativeClient {
            pool: Rc::new(RefCell::new(LocalPool::new())),
            ctx: Rc::new(RefCell::new(NativeClientContext(ctx))),
        },
        &env,
        None,
        None,
    );
    let sdk = Rc::new(sdk);
    let mut spawner = sdk.client.pool.borrow_mut().spawner();
    {
        let sdk = sdk.clone();

        spawner
            .spawn_local(async move {
                sdk.initialize().await.unwrap();
            })
            .unwrap();
    }

    sdk.client.pool.borrow_mut().run_until_stalled();

    Box::new(CppSDK { sdk })
}

impl CppSDK {
    fn refresh_order(
        &self,
        callback: RefreshOrderCallback,
        callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
        order_id: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        spawner
            .spawn_local(refresh_order_task(
                self.sdk.clone(),
                callback,
                callback_state,
                order_id,
            ))
            .unwrap();

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }

    fn fetch_order_credentials(
        &self,
        callback: FetchOrderCredentialsCallback,
        callback_state: UniquePtr<ffi::FetchOrderCredentialsCallbackState>,
        order_id: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        spawner
            .spawn_local(fetch_order_credentials_task(
                self.sdk.clone(),
                callback,
                callback_state,
                order_id,
            ))
            .unwrap();

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
        spawner
            .spawn_local(prepare_credentials_presentation_task(
                self.sdk.clone(),
                callback,
                callback_state,
                domain,
                path,
            ))
            .unwrap();

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }

    fn credential_summary(
        &self,
        callback: CredentialSummaryCallback,
        callback_state: UniquePtr<ffi::CredentialSummaryCallbackState>,
        domain: String,
    ) {
        let mut spawner = self.sdk.client.pool.borrow_mut().spawner();
        spawner
            .spawn_local(credential_summary_task(
                self.sdk.clone(),
                callback,
                callback_state,
                domain,
            ))
            .unwrap();

        self.sdk.client.pool.borrow_mut().run_until_stalled();
    }
}

#[repr(transparent)]
pub struct RefreshOrderCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::RefreshOrderCallbackState,
        result: ffi::RewardsResult,
        order: &str,
    ),
);

unsafe impl ExternType for RefreshOrderCallback {
    type Id = type_id!("brave_rewards::RefreshOrderCallback");
    type Kind = cxx::kind::Trivial;
}

async fn refresh_order_task(
    sdk: Rc<brave_rewards::sdk::SDK<NativeClient>>,
    callback: RefreshOrderCallback,
    callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
    order_id: String,
) {
    match sdk.refresh_order(&order_id).await {
        Ok(order) => {
            let order = serde_json::to_string(&order).unwrap();
            callback.0(callback_state.into_raw(), ffi::RewardsResult::Ok, &order)
        }
        Err(e) => callback.0(
            callback_state.into_raw(),
            e.source()
                .unwrap()
                .downcast_ref::<brave_rewards::errors::InternalError>()
                .unwrap()
                .into(),
            "",
        ),
    }
}

#[repr(transparent)]
pub struct FetchOrderCredentialsCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::FetchOrderCredentialsCallbackState,
        result: ffi::RewardsResult,
    ),
);

unsafe impl ExternType for FetchOrderCredentialsCallback {
    type Id = type_id!("brave_rewards::FetchOrderCredentialsCallback");
    type Kind = cxx::kind::Trivial;
}

async fn fetch_order_credentials_task(
    sdk: Rc<brave_rewards::sdk::SDK<NativeClient>>,
    callback: FetchOrderCredentialsCallback,
    callback_state: UniquePtr<ffi::FetchOrderCredentialsCallbackState>,
    order_id: String,
) {
    match sdk.fetch_order_credentials(&order_id).await {
        Ok(_) => callback.0(callback_state.into_raw(), ffi::RewardsResult::Ok),
        Err(e) => callback.0(
            callback_state.into_raw(),
            e.source()
                .unwrap()
                .downcast_ref::<brave_rewards::errors::InternalError>()
                .unwrap()
                .into(),
        ),
    }
}

#[repr(transparent)]
pub struct PrepareCredentialsPresentationCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::PrepareCredentialsPresentationCallbackState,
        result: ffi::RewardsResult,
        presentation: &str,
    ),
);

unsafe impl ExternType for PrepareCredentialsPresentationCallback {
    type Id = type_id!("brave_rewards::PrepareCredentialsPresentationCallback");
    type Kind = cxx::kind::Trivial;
}

async fn prepare_credentials_presentation_task(
    sdk: Rc<brave_rewards::sdk::SDK<NativeClient>>,
    callback: PrepareCredentialsPresentationCallback,
    callback_state: UniquePtr<ffi::PrepareCredentialsPresentationCallbackState>,
    domain: String,
    path: String,
) {
    match sdk.prepare_credentials_presentation(&domain, &path).await {
        Ok(Some(presentation)) => callback.0(
            callback_state.into_raw(),
            ffi::RewardsResult::Ok,
            &presentation,
        ),
        Ok(None) => callback.0(callback_state.into_raw(), ffi::RewardsResult::Ok, ""),
        Err(e) => callback.0(
            callback_state.into_raw(),
            e.source()
                .unwrap()
                .downcast_ref::<brave_rewards::errors::InternalError>()
                .unwrap()
                .into(),
            "",
        ),
    }
}

#[repr(transparent)]
pub struct CredentialSummaryCallback(
    pub  extern "C" fn(
        callback_state: *mut ffi::CredentialSummaryCallbackState,
        result: ffi::RewardsResult,
        summary: &str,
    ),
);

unsafe impl ExternType for CredentialSummaryCallback {
    type Id = type_id!("brave_rewards::CredentialSummaryCallback");
    type Kind = cxx::kind::Trivial;
}

async fn credential_summary_task(
    sdk: Rc<brave_rewards::sdk::SDK<NativeClient>>,
    callback: CredentialSummaryCallback,
    callback_state: UniquePtr<ffi::CredentialSummaryCallbackState>,
    domain: String,
) {
    match sdk.matching_credential_summary(&domain).await {
        Ok(Some(summary)) => {
            let summary = serde_json::to_string(&summary).unwrap();
            callback.0(callback_state.into_raw(), ffi::RewardsResult::Ok, &summary)
        }
        Ok(None) => callback.0(callback_state.into_raw(), ffi::RewardsResult::Ok, ""),
        Err(e) => callback.0(
            callback_state.into_raw(),
            e.source()
                .unwrap()
                .downcast_ref::<brave_rewards::errors::InternalError>()
                .unwrap()
                .into(),
            "",
        ),
    }
}
