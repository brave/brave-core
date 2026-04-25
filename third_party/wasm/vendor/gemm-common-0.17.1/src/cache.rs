#[derive(Default, Debug, Copy, Clone)]
pub struct CacheInfo {
    pub associativity: usize,
    pub cache_bytes: usize,
    pub cache_line_bytes: usize,
}

#[derive(Default, Debug, Copy, Clone)]
pub struct KernelParams {
    pub kc: usize,
    pub mc: usize,
    pub nc: usize,
}

pub trait DivCeil: Sized {
    fn msrv_div_ceil(self, rhs: Self) -> Self;
    fn msrv_next_multiple_of(self, rhs: Self) -> Self;
    fn msrv_checked_next_multiple_of(self, rhs: Self) -> Option<Self>;
}

impl DivCeil for usize {
    #[inline]
    fn msrv_div_ceil(self, rhs: Self) -> Self {
        let d = self / rhs;
        let r = self % rhs;
        if r > 0 {
            d + 1
        } else {
            d
        }
    }

    #[inline]
    fn msrv_next_multiple_of(self, rhs: Self) -> Self {
        match self % rhs {
            0 => self,
            r => self + (rhs - r),
        }
    }

    #[inline]
    fn msrv_checked_next_multiple_of(self, rhs: Self) -> Option<Self> {
        {
            match self.checked_rem(rhs)? {
                0 => Some(self),
                r => self.checked_add(rhs - r),
            }
        }
    }
}

#[cfg(target_vendor = "apple")]
fn has_amx_impl() -> bool {
    if !cfg!(miri) {
        #[cfg(feature = "std")]
        {
            use sysctl::Ctl;
            use sysctl::Sysctl;

            if let Ok(brand) =
                Ctl::new("machdep.cpu.brand_string").and_then(|ctl| ctl.value_string())
            {
                let mut words = brand.split_whitespace();
                let apple = words.next();
                let mx = words.next();
                return apple == Some("Apple") && matches!(mx, Some("M1" | "M2" | "M3"));
            }
        }
    }
    false
}

fn cache_info() -> Option<[CacheInfo; 3]> {
    if !cfg!(miri) {
        #[cfg(feature = "std")]
        {
            #[cfg(target_os = "linux")]
            {
                use std::fs;
                fn try_cache_info_linux() -> Result<[CacheInfo; 3], std::io::Error> {
                    let mut all_info = [CacheInfo {
                        associativity: 8,
                        cache_bytes: 0,
                        cache_line_bytes: 64,
                    }; 3];

                    for cpu_x in fs::read_dir("/sys/devices/system/cpu")? {
                        let cpu_x = cpu_x?.path();
                        let Some(cpu_x_name) = cpu_x.file_name().and_then(|f| f.to_str()) else {
                            continue;
                        };
                        if !cpu_x_name.starts_with("cpu") {
                            continue;
                        }
                        let cache = cpu_x.join("cache");
                        if !cache.is_dir() {
                            continue;
                        }
                        'index: for index_y in fs::read_dir(cache)? {
                            let index_y = index_y?.path();
                            if !index_y.is_dir() {
                                continue;
                            }
                            let Some(index_y_name) = index_y.file_name().and_then(|f| f.to_str())
                            else {
                                continue;
                            };
                            if !index_y_name.starts_with("index") {
                                continue;
                            }

                            let mut cache_info = CacheInfo {
                                associativity: 8,
                                cache_bytes: 0,
                                cache_line_bytes: 64,
                            };
                            let mut level: usize = 0;
                            let mut shared_count: usize = 0;

                            for entry in fs::read_dir(index_y)? {
                                let entry = entry?.path();
                                if let Some(name) = entry.file_name() {
                                    let contents = fs::read_to_string(&entry)?;
                                    let contents = contents.trim();
                                    if name == "type" && !matches!(contents, "Data" | "Unified") {
                                        continue 'index;
                                    }
                                    if name == "shared_cpu_list" {
                                        for item in contents.split(',') {
                                            if item.contains('-') {
                                                let mut item = item.split('-');
                                                let Some(start) = item.next() else {
                                                    continue 'index;
                                                };
                                                let Some(end) = item.next() else {
                                                    continue 'index;
                                                };

                                                let Ok(start) = start.parse::<usize>() else {
                                                    continue 'index;
                                                };
                                                let Ok(end) = end.parse::<usize>() else {
                                                    continue 'index;
                                                };

                                                shared_count += end + 1 - start;
                                            } else {
                                                shared_count += 1;
                                            }
                                        }
                                    }

                                    if name == "level" {
                                        let Ok(contents) = contents.parse::<usize>() else {
                                            continue 'index;
                                        };
                                        level = contents;
                                    }

                                    if name == "coherency_line_size" {
                                        let Ok(contents) = contents.parse::<usize>() else {
                                            continue 'index;
                                        };
                                        cache_info.cache_line_bytes = contents;
                                    }
                                    if name == "ways_of_associativity" {
                                        let Ok(contents) = contents.parse::<usize>() else {
                                            continue 'index;
                                        };
                                        cache_info.associativity = contents;
                                    }
                                    if name == "size" {
                                        if contents.ends_with("G") {
                                            let Ok(contents) =
                                                contents.trim_end_matches('G').parse::<usize>()
                                            else {
                                                continue 'index;
                                            };
                                            cache_info.cache_bytes = contents * 1024 * 1024 * 1024;
                                        } else if contents.ends_with("M") {
                                            let Ok(contents) =
                                                contents.trim_end_matches('M').parse::<usize>()
                                            else {
                                                continue 'index;
                                            };
                                            cache_info.cache_bytes = contents * 1024 * 1024;
                                        } else if contents.ends_with("K") {
                                            let Ok(contents) =
                                                contents.trim_end_matches('K').parse::<usize>()
                                            else {
                                                continue 'index;
                                            };
                                            cache_info.cache_bytes = contents * 1024;
                                        } else {
                                            let Ok(contents) = contents.parse::<usize>() else {
                                                continue 'index;
                                            };
                                            cache_info.cache_bytes = contents;
                                        }
                                    }
                                }
                            }
                            if level == 3 {
                                shared_count = 1;
                            }
                            if level > 0 {
                                if cache_info.cache_line_bytes
                                    >= all_info[level - 1].cache_line_bytes
                                {
                                    all_info[level - 1].associativity = cache_info.associativity;
                                    all_info[level - 1].cache_line_bytes =
                                        cache_info.cache_line_bytes;
                                    all_info[level - 1].cache_bytes =
                                        cache_info.cache_bytes / shared_count;
                                }
                            }
                        }
                    }

                    for (info, default) in core::iter::zip(&mut all_info, CACHE_INFO_DEFAULT) {
                        if info.cache_bytes == 0 {
                            *info = default;
                        }
                    }

                    Ok(all_info)
                }
                if let Ok(info) = try_cache_info_linux() {
                    return Some(info);
                }

                if let Ok(lscpu) = std::process::Command::new("lscpu")
                    .arg("-B")
                    .arg("-C=type,level,ways,coherency-size,one-size")
                    .output()
                {
                    if lscpu.status.success() {
                        if let Ok(lscpu) = String::from_utf8(lscpu.stdout).as_deref() {
                            let mut info = CACHE_INFO_DEFAULT;
                            for line in lscpu.lines().skip(1) {
                                let mut iter = line.split_whitespace();
                                if let [Some(cache_type), Some(level), Some(ways), Some(coherency_size), Some(one_size)] = [
                                    iter.next(),
                                    iter.next(),
                                    iter.next(),
                                    iter.next(),
                                    iter.next(),
                                ] {
                                    if let "Data" | "Unified" = cache_type {
                                        let level = level.parse::<usize>().unwrap();
                                        let ways = ways.parse::<usize>().unwrap();
                                        let coherency_size =
                                            coherency_size.parse::<usize>().unwrap();
                                        let one_size = one_size.parse::<usize>().unwrap();

                                        info[level - 1].associativity = ways;
                                        info[level - 1].cache_line_bytes = coherency_size;
                                        info[level - 1].cache_bytes = one_size;
                                    }
                                }
                            }
                            return Some(info);
                        }
                    }
                }
            }
            #[cfg(target_vendor = "apple")]
            {
                use sysctl::Ctl;
                use sysctl::Sysctl;

                let mut all_info = CACHE_INFO_DEFAULT;
                if let Ok(l1) =
                    Ctl::new("hw.perflevel0.l1dcachesize").and_then(|ctl| ctl.value_string())
                {
                    if let Ok(l1) = l1.parse::<usize>() {
                        all_info[0].cache_bytes = l1;
                    }
                }
                if let (Ok(physicalcpu), Ok(cpusperl2), Ok(l2)) = (
                    Ctl::new("hw.perflevel0.physicalcpu").and_then(|ctl| ctl.value_string()),
                    Ctl::new("hw.perflevel0.cpusperl2").and_then(|ctl| ctl.value_string()),
                    Ctl::new("hw.perflevel0.l2cachesize").and_then(|ctl| ctl.value_string()),
                ) {
                    if let (Ok(_physicalcpu), Ok(cpusperl2), Ok(l2)) = (
                        physicalcpu.parse::<usize>(),
                        cpusperl2.parse::<usize>(),
                        l2.parse::<usize>(),
                    ) {
                        all_info[1].cache_bytes = l2 / cpusperl2;
                    }
                }
                all_info[2].cache_bytes = 0;
                return Some(all_info);
            }
        }

        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        {
            use raw_cpuid::CpuId;
            let cpuid = CpuId::new();

            if let Some(vf) = cpuid.get_vendor_info() {
                let vf = vf.as_str();
                if vf == "GenuineIntel" {
                    if let Some(cparams) = cpuid.get_cache_parameters() {
                        // not sure why, intel cpus seem to prefer smaller mc
                        let mut info = [CacheInfo {
                            cache_bytes: 0,
                            associativity: 0,
                            cache_line_bytes: 64,
                        }; 3];

                        for cache in cparams {
                            use raw_cpuid::CacheType::*;
                            match cache.cache_type() {
                                Null | Instruction | Reserved => continue,
                                Data | Unified => {
                                    let level = cache.level() as usize;
                                    let associativity = cache.associativity();
                                    let nsets = cache.sets();
                                    let cache_line_bytes = cache.coherency_line_size();
                                    if level > 0 && level < 4 {
                                        let info = &mut info[level - 1];
                                        info.cache_line_bytes = cache_line_bytes;
                                        info.associativity = associativity;
                                        info.cache_bytes = associativity * nsets * cache_line_bytes;
                                    }
                                }
                            }
                        }
                        return Some(info);
                    }
                }

                if vf == "AuthenticAMD" {
                    if let Some(l1) = cpuid.get_l1_cache_and_tlb_info() {
                        if let Some(l23) = cpuid.get_l2_l3_cache_and_tlb_info() {
                            let compute_info = |associativity: raw_cpuid::Associativity,
                                                cache_kb: usize,
                                                cache_line_bytes: u8|
                             -> CacheInfo {
                                let cache_bytes = cache_kb as usize * 1024;
                                let cache_line_bytes = cache_line_bytes as usize;

                                use raw_cpuid::Associativity::*;
                                let associativity = match associativity {
                                    Unknown | Disabled => {
                                        return CacheInfo {
                                            associativity: 0,
                                            cache_bytes: 0,
                                            cache_line_bytes: 64,
                                        }
                                    }
                                    FullyAssociative => cache_bytes / cache_line_bytes,
                                    DirectMapped => 1,
                                    NWay(n) => n as usize,
                                };

                                CacheInfo {
                                    associativity,
                                    cache_bytes,
                                    cache_line_bytes,
                                }
                            };
                            return Some([
                                compute_info(
                                    l1.dcache_associativity(),
                                    l1.dcache_size() as usize,
                                    l1.dcache_line_size(),
                                ),
                                compute_info(
                                    l23.l2cache_associativity(),
                                    l23.l2cache_size() as usize,
                                    l23.l2cache_line_size(),
                                ),
                                compute_info(
                                    l23.l3cache_associativity(),
                                    l23.l3cache_size() as usize * 512,
                                    l23.l3cache_line_size(),
                                ),
                            ]);
                        }
                    }
                }
            }
        }
    }
    None
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
static CACHE_INFO_DEFAULT: [CacheInfo; 3] = [
    CacheInfo {
        associativity: 8,
        cache_bytes: 32 * 1024, // 32KiB
        cache_line_bytes: 64,
    },
    CacheInfo {
        associativity: 8,
        cache_bytes: 256 * 1024, // 256KiB
        cache_line_bytes: 64,
    },
    CacheInfo {
        associativity: 8,
        cache_bytes: 2 * 1024 * 1024, // 2MiB
        cache_line_bytes: 64,
    },
];

#[cfg(any(target_arch = "powerpc", target_arch = "powerpc64"))]
static CACHE_INFO_DEFAULT: [CacheInfo; 3] = [
    CacheInfo {
        associativity: 8,
        cache_bytes: 64 * 1024, // 64KiB
        cache_line_bytes: 64,
    },
    CacheInfo {
        associativity: 8,
        cache_bytes: 512 * 1024, // 512KiB
        cache_line_bytes: 64,
    },
    CacheInfo {
        associativity: 8,
        cache_bytes: 4 * 1024 * 1024, // 4MiB
        cache_line_bytes: 64,
    },
];

#[cfg(not(any(
    target_arch = "powerpc",
    target_arch = "powerpc64",
    target_arch = "x86",
    target_arch = "x86_64"
)))]
static CACHE_INFO_DEFAULT: [CacheInfo; 3] = [
    CacheInfo {
        associativity: 8,
        cache_bytes: 16 * 1024, // 16KiB
        cache_line_bytes: 64,
    },
    CacheInfo {
        associativity: 8,
        cache_bytes: 512 * 1024, // 512KiB
        cache_line_bytes: 64,
    },
    CacheInfo {
        associativity: 8,
        cache_bytes: 1024 * 1024, // 1MiB
        cache_line_bytes: 64,
    },
];

pub struct CacheInfoDeref;
#[cfg(target_vendor = "apple")]
pub struct HasAmx;

impl core::ops::Deref for CacheInfoDeref {
    type Target = [CacheInfo; 3];

    #[inline]
    fn deref(&self) -> &Self::Target {
        #[cfg(not(feature = "std"))]
        {
            static CACHE_INFO: once_cell::race::OnceBox<[CacheInfo; 3]> =
                once_cell::race::OnceBox::new();
            CACHE_INFO
                .get_or_init(|| alloc::boxed::Box::new(cache_info().unwrap_or(CACHE_INFO_DEFAULT)))
        }
        #[cfg(feature = "std")]
        {
            static CACHE_INFO: once_cell::sync::OnceCell<[CacheInfo; 3]> =
                once_cell::sync::OnceCell::new();
            CACHE_INFO.get_or_init(|| cache_info().unwrap_or(CACHE_INFO_DEFAULT))
        }
    }
}

#[cfg(target_vendor = "apple")]
impl HasAmx {
    #[inline]
    pub fn get() -> bool {
        static HAS_AMX: core::sync::atomic::AtomicU8 = core::sync::atomic::AtomicU8::new(u8::MAX);
        let mut has_amx = HAS_AMX.load(::core::sync::atomic::Ordering::Relaxed);
        if has_amx == u8::MAX {
            let b = has_amx_impl() as u8;
            HAS_AMX.store(b, core::sync::atomic::Ordering::Relaxed);
            has_amx = b;
        }
        has_amx != 0
    }
}

pub static CACHE_INFO: CacheInfoDeref = CacheInfoDeref;

#[inline]
fn gcd(mut a: usize, mut b: usize) -> usize {
    while b != 0 {
        let rem = a % b;
        a = b;
        b = rem;
    }
    a
}

#[inline]
fn round_down(a: usize, b: usize) -> usize {
    a / b * b
}

pub fn kernel_params(
    m: usize,
    n: usize,
    k: usize,
    mr: usize,
    nr: usize,
    sizeof: usize,
) -> KernelParams {
    if m == 0 || n == 0 || k == 0 {
        return KernelParams {
            kc: k,
            mc: m,
            nc: n,
        };
    }

    let info = *CACHE_INFO;

    let l1_cache_bytes = info[0].cache_bytes.max(32 * 1024);
    let l2_cache_bytes = info[1].cache_bytes;
    let l3_cache_bytes = info[2].cache_bytes;

    let l1_line_bytes = info[0].cache_line_bytes.max(64);

    let l1_assoc = info[0].associativity.max(2);
    let l2_assoc = info[1].associativity.max(2);
    let l3_assoc = info[2].associativity.max(2);

    let l1_n_sets = l1_cache_bytes / (l1_line_bytes * l1_assoc);

    // requires
    // A micropanels must occupy different cache sets
    // so that loading a micropanel evicts the previous one
    // => byte stride must be multiple of n_sets×line_bytes
    //
    // => mr×kc×scalar_bytes == C_A × l1_line_bytes × l1_n_sets
    //
    // l1 must be able to hold A micropanel, B micropanel
    //
    // => C_A + C_B <= l1_assoc

    // a×n = b×m
    // find lcm of a, b
    // n = lcm / a = b/gcd(a,b)
    // m = lcm / b = a/gcd(a,b)

    let gcd = gcd(mr * sizeof, l1_line_bytes * l1_n_sets);
    let kc_0 = (l1_line_bytes * l1_n_sets) / gcd;
    let c_lhs = (mr * sizeof) / gcd;
    let c_rhs = (nr * kc_0 * sizeof) / (l1_line_bytes * l1_n_sets);
    let kc_multiplier = l1_assoc / (c_lhs + c_rhs);
    // let auto_kc = kc_0 * kc_multiplier;
    let auto_kc = (kc_0 * kc_multiplier.next_power_of_two()).max(512).min(k);
    let k_iter = k.msrv_div_ceil(auto_kc);
    let auto_kc = k.msrv_div_ceil(k_iter);

    // l2 cache must hold
    //  - B micropanel: nr×kc: assume 1 assoc degree
    //  - A macropanel: mc×kc
    // mc×kc×scalar_bytes
    let auto_mc = if l2_cache_bytes == 0 {
        panic!();
    } else {
        let rhs_micropanel_bytes = nr * auto_kc * sizeof;
        let rhs_l2_assoc = rhs_micropanel_bytes.msrv_div_ceil(l2_cache_bytes / l2_assoc);
        let lhs_l2_assoc = (l2_assoc - 1 - rhs_l2_assoc).max(1);

        let mc_from_lhs_l2_assoc = |lhs_l2_assoc: usize| -> usize {
            (lhs_l2_assoc * l2_cache_bytes) / (l2_assoc * sizeof * auto_kc)
        };

        let auto_mc = round_down(mc_from_lhs_l2_assoc(lhs_l2_assoc), mr);
        let m_iter = m.msrv_div_ceil(auto_mc);
        m.msrv_div_ceil(m_iter * mr) * mr
    };
    let auto_mc = Ord::min(auto_mc, 8 * mr);

    // l3 cache must hold
    //  - A macropanel: mc×kc: assume 1 assoc degree
    //  - B macropanel: nc×kc
    let auto_nc = if l3_cache_bytes == 0 {
        0
    } else {
        // let lhs_macropanel_bytes = auto_mc * auto_kc * sizeof;
        // let lhs_l3_assoc = msrv_div_ceil(lhs_macropanel_bytes, l3_cache_bytes / l3_assoc);
        let rhs_l3_assoc = l3_assoc - 1;
        let rhs_macropanel_max_bytes = (rhs_l3_assoc * l3_cache_bytes) / l3_assoc;

        let auto_nc = round_down(rhs_macropanel_max_bytes / (sizeof * auto_kc), nr);
        let n_iter = n.msrv_div_ceil(auto_nc);
        n.msrv_div_ceil(n_iter * nr) * nr
    };

    KernelParams {
        kc: auto_kc,
        mc: auto_mc,
        nc: auto_nc,
    }
}
