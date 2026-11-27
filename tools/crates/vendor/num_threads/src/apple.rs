use std::num::NonZeroUsize;

use libc::{
    kern_return_t, mach_port_t, natural_t, task_threads, thread_act_array_t, vm_deallocate,
};

#[allow(non_camel_case_types)]
// https://developer.apple.com/documentation/kernel/mach_port_name_t
type mach_port_name_t = natural_t;

extern "C" {
    static mut mach_task_self_: mach_port_t;

    // https://developer.apple.com/documentation/kernel/1578777-mach_port_deallocate
    fn mach_port_deallocate(task: mach_port_t, name: mach_port_name_t) -> kern_return_t;
}

pub(crate) fn num_threads() -> Option<NonZeroUsize> {
    // https://developer.apple.com/documentation/kernel/1537751-task_threads
    let mut thread_list: thread_act_array_t = std::ptr::null_mut();
    let mut thread_count = 0;

    // Safety:
    //  - `mach_task_self_` contains the current task's valid port,
    //  - `thread_list` is a pointer that will point to kernel allocated memory that needs to be
    //    deallocated if the call succeeds
    let task = unsafe { mach_task_self_ };
    let result = unsafe { task_threads(task, &mut thread_list, &mut thread_count) };

    if result == libc::KERN_SUCCESS {
        // Deallocate the mach port rights for the threads
        for thread in 0..thread_count {
            unsafe {
                mach_port_deallocate(task, *(thread_list.offset(thread as isize)) as u32);
            }
        }
        // Deallocate the thread list
        unsafe {
            vm_deallocate(
                task,
                thread_list as usize,
                std::mem::size_of::<mach_port_t>() * thread_count as usize,
            );
        }

        let rosetta_thread_count = {
            #[cfg(target_os = "macos")]
            {
                let mut translated: libc::c_int = 0;
                let mut size = std::mem::size_of_val(&translated);
                let result = unsafe {
                    libc::sysctlbyname(
                        b"sysctl.proc_translated\0".as_ptr() as *const libc::c_char,
                        &mut translated as *mut _ as *mut libc::c_void,
                        &mut size,
                        std::ptr::null_mut(),
                        0,
                    )
                };

                // Rosetta's exception-server thread is included by task_threads, but is not an
                // application thread.
                if result == 0 && translated == 1 { 1 } else { 0 }
            }

            #[cfg(not(target_os = "macos"))]
            {
                0
            }
        };

        (thread_count as usize)
            .checked_sub(rosetta_thread_count)
            .and_then(NonZeroUsize::new)
    } else {
        None
    }
}
