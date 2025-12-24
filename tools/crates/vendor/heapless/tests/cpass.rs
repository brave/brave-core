//! Collections of `Send`-able things are `Send`

use heapless::{
    spsc::{Consumer, Producer, Queue},
    HistoryBuffer, Vec,
};

#[test]
fn send() {
    struct IsSend;

    unsafe impl Send for IsSend {}

    fn is_send<T>()
    where
        T: Send,
    {
    }

    is_send::<Consumer<IsSend, 4>>();
    is_send::<Producer<IsSend, 4>>();
    is_send::<Queue<IsSend, 4>>();
    is_send::<Vec<IsSend, 4>>();
    is_send::<HistoryBuffer<IsSend, 4>>();
}
