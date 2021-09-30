mod httpclient;
mod storage;

pub use brave_rewards;
use cxx::{type_id, ExternType, UniquePtr};
use futures::executor::LocalPool;
use futures::task::LocalSpawnExt;
use std::cell::RefCell;
use std::rc::Rc;

use crate::httpclient::HttpRoundtripContext;

pub struct NativeClient();

thread_local! {
    static LOCAL_POOL: RefCell<LocalPool> = RefCell::new(LocalPool::new());
}

#[allow(unused)]
#[cxx::bridge(namespace = brave_rewards)]
mod ffi {
    #[derive(Debug)]
    pub enum RewardsResult {
        Ok,
        /// An error occured when converting from a `CompressedRistretto` to a `RistrettoPoint`
        PointDecompressionError,
        /// An error occured when interpretting bytes as a scalar
        ScalarFormatError,
        /// An error in the length of bytes handed to a constructor.
        BytesLengthError,
        /// Verification failed
        VerifyError,
        /// Inputs differed in length
        LengthMismatchError,
        /// Decoding failed
        DecodingError,
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

        type CppSDK;
        fn initialize_sdk(env: String) -> Box<CppSDK>;
        fn refresh_order(
            self: &CppSDK,
            callback: RefreshOrderCallback,
            callback_state: UniquePtr<RefreshOrderCallbackState>,
            order_id: String,
        );
    }

    unsafe extern "C++" {
        include!("shim.hpp");

        fn shim_executeRequest(
            req: &HttpRequest,
            done: fn(Box<HttpRoundtripContext>, resp: HttpResponse),
            ctx: Box<HttpRoundtripContext>,
        );

        fn shim_scheduleWakeup(delay_ms: u64, done: fn());

        type RefreshOrderCallbackState;
        type RefreshOrderCallback = crate::RefreshOrderCallback;
    }
}

pub struct CppSDK {
    sdk: Rc<brave_rewards::sdk::SDK>,
}

fn initialize_sdk(env: String) -> Box<CppSDK> {
    // FIXME replace with ffi logging
    tracing_subscriber::fmt::init();

    let sdk = brave_rewards::sdk::SDK::new::<NativeClient>(&env, None, None);
    let sdk = Rc::new(sdk);
    LOCAL_POOL.with(|pool| {
        let sdk = sdk.clone();
        let mut spawner = pool.borrow_mut().spawner();

        spawner
            .spawn_local(async move {
                sdk.initialize::<NativeClient>().await.unwrap();
            })
            .unwrap();

        pool.borrow_mut().run_until_stalled();
    });

    Box::new(CppSDK { sdk })
}

impl CppSDK {
    fn refresh_order(
        &self,
        callback: RefreshOrderCallback,
        callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
        order_id: String,
    ) {
        LOCAL_POOL.with(|pool| {
            let mut spawner = pool.borrow_mut().spawner();

            spawner
                .spawn_local(refresh_order_task(
                    self.sdk.clone(),
                    callback,
                    callback_state,
                    order_id,
                ))
                .unwrap();

            pool.borrow_mut().run_until_stalled();
        })
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
    sdk: Rc<brave_rewards::sdk::SDK>,
    callback: RefreshOrderCallback,
    callback_state: UniquePtr<ffi::RefreshOrderCallbackState>,
    order_id: String,
) {
    let order = sdk.refresh_order::<NativeClient>(&order_id).await;
    let order = serde_json::to_string(&order.unwrap()).unwrap();

    callback.0(callback_state.into_raw(), ffi::RewardsResult::Ok, &order);
}
