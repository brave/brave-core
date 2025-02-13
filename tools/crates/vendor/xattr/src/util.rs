use std::io;

pub fn extract_noattr(result: io::Result<Vec<u8>>) -> io::Result<Option<Vec<u8>>> {
    result.map(Some).or_else(|e| {
        if e.raw_os_error() == Some(crate::sys::ENOATTR) {
            Ok(None)
        } else {
            Err(e)
        }
    })
}

#[allow(dead_code)]
pub fn allocate_loop<E, F: FnMut(&mut [u8]) -> Result<usize, E>>(mut f: F) -> io::Result<Vec<u8>>
where
    io::Error: From<E>,
{
    const ERANGE: i32 = rustix::io::Errno::RANGE.raw_os_error();

    let mut vec: Vec<u8> = Vec::new();
    loop {
        let ret = f(&mut [])?;
        vec.resize(ret, 0);

        match f(&mut vec) {
            Ok(size) => {
                vec.truncate(size);
                vec.shrink_to_fit();
                return Ok(vec);
            }

            Err(e) => {
                let err: io::Error = e.into();
                if err.raw_os_error() == Some(ERANGE) {
                    continue;
                } else {
                    return Err(err);
                }
            }
        }
    }
}
