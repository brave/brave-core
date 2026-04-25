use std::io;
use std::net::UdpSocket;

pub trait WritevExt {
    fn writev(&self, bufs: [&[u8]; 2]) -> io::Result<usize>;
    fn readv(&self, bufs: [&mut [u8]; 2]) -> io::Result<usize>;
}

#[cfg(unix)]
mod imp {
    use libc;
    use std::os::unix::io::AsRawFd;

    use super::*;

    impl WritevExt for UdpSocket {
        fn writev(&self, bufs: [&[u8]; 2]) -> io::Result<usize> {
            unsafe {
                let iovecs = [
                    libc::iovec {
                        iov_base: bufs[0].as_ptr() as *const _ as *mut _,
                        iov_len: bufs[0].len(),
                    },
                    libc::iovec {
                        iov_base: bufs[1].as_ptr() as *const _ as *mut _,
                        iov_len: bufs[1].len(),
                    },
                ];
                let r = libc::writev(self.as_raw_fd(), iovecs.as_ptr(), 2);
                if r < 0 {
                    Err(io::Error::last_os_error())
                } else {
                    Ok(r as usize)
                }
            }
        }

        fn readv(&self, bufs: [&mut [u8]; 2]) -> io::Result<usize> {
            unsafe {
                let mut iovecs = [
                    libc::iovec {
                        iov_base: bufs[0].as_mut_ptr() as *mut _,
                        iov_len: bufs[0].len(),
                    },
                    libc::iovec {
                        iov_base: bufs[1].as_mut_ptr() as *mut _,
                        iov_len: bufs[1].len(),
                    },
                ];
                let r = libc::readv(self.as_raw_fd(), iovecs.as_mut_ptr(), 2);
                if r < 0 {
                    Err(io::Error::last_os_error())
                } else {
                    Ok(r as usize)
                }
            }
        }
    }
}

#[cfg(windows)]
mod imp {
    use winapi::um::winsock2;
    use winapi::shared::ws2def;
    use winapi::shared::minwindef;
    use std::os::windows::io::AsRawSocket;
    use std::ptr;

    use super::*;

    impl WritevExt for UdpSocket {
        fn writev(&self, bufs: [&[u8]; 2]) -> io::Result<usize> {
            unsafe {
                let mut wsabufs = [
                    ws2def::WSABUF {
                        len: bufs[0].len() as winsock2::u_long,
                        buf: bufs[0].as_ptr() as *const _ as *mut _,
                    },
                    ws2def::WSABUF {
                        len: bufs[1].len() as winsock2::u_long,
                        buf: bufs[1].as_ptr() as *const _ as *mut _,
                    },
                ];
                let mut sent = 0;
                let r = winsock2::WSASend(
                    self.as_raw_socket() as usize,
                    wsabufs.as_mut_ptr(),
                    bufs.len() as minwindef::DWORD,
                    &mut sent,
                    0,
                    ptr::null_mut(),
                    None,
                );
                if r == 0 {
                    Ok(sent as usize)
                } else {
                    Err(io::Error::last_os_error())
                }
            }
        }

        fn readv(&self, bufs: [&mut [u8]; 2]) -> io::Result<usize> {
            unsafe {
                let mut wsabufs = [
                    ws2def::WSABUF {
                        len: bufs[0].len() as winsock2::u_long,
                        buf: bufs[0].as_mut_ptr() as *mut _,
                    },
                    ws2def::WSABUF {
                        len: bufs[1].len() as winsock2::u_long,
                        buf: bufs[1].as_mut_ptr() as *mut _,
                    },
                ];
                let mut recved = 0;
                let mut flags = 0;
                let r = winsock2::WSARecv(
                    self.as_raw_socket() as usize,
                    wsabufs.as_mut_ptr(),
                    bufs.len() as minwindef::DWORD,
                    &mut recved,
                    &mut flags,
                    ptr::null_mut(),
                    None,
                );
                if r == 0 {
                    Ok(recved as usize)
                } else {
                    Err(io::Error::last_os_error())
                }
            }
        }
    }
}
