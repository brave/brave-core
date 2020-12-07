#[allow(unused_imports)]
use serde::Deserialize;
#[allow(unused_imports)]
use std::collections::HashMap;
#[allow(unused_macros)]
macro_rules! ternary {
  ($c:expr, $v:expr, $v1:expr) => {
    if $c { $v } else { $v1 } };
  }

pub fn exec() -> bool {
  #[cfg(target_os = "windows")]
  return wexec();
  #[allow(unreachable_code)]
  { return false; }
}

#[cfg(target_os = "windows")]
fn wexec() -> bool {
  let com_con = match wmi::COMLibrary::new() {
    Ok(c) => c,
    Err(_) => return false,
  };
  let wmi_con = match wmi::WMIConnection::new(com_con.into()) {
    Ok(c) => c,
    Err(_) => return false,
  };
  wpcon(&wmi_con)
}

#[cfg(target_os = "windows")]
fn wpcon(c: &wmi::WMIConnection) -> bool {
  c.raw_query("SELECT * FROM Win32_PortConnector")
    .map_or(false, |r| {
      let r: &Vec<HashMap<String, wmi::Variant>> = &r;
      ternary!(r.len() > 0, false, true)
  })
}
