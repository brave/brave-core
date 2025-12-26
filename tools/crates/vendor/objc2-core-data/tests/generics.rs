#![cfg(all(
    feature = "NSFetchRequest",
    feature = "NSPersistentStoreRequest",
    feature = "NSManagedObject"
))]
use objc2_core_data::{NSFetchRequest, NSManagedObject, NSManagedObjectID};

#[test]
fn construct_and_use() {
    let fetch_request = unsafe { NSFetchRequest::<NSManagedObject>::new() };
    let _fetch_request = unsafe { fetch_request.cast_unchecked::<NSManagedObjectID>() };
}
