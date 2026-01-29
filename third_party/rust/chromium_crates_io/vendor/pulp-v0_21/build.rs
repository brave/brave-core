use std::path::Path;
use std::{env, fs};

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
enum ArchX86 {
	_32,
	_64,
}

impl ArchX86 {
	fn reg_prefix(self) -> &'static str {
		match self {
			ArchX86::_32 => "e",
			ArchX86::_64 => "r",
		}
	}
}

fn load_f32x4(out: usize, offset: usize, start: i32, end: i32, arch: ArchX86) -> String {
	let p = arch.reg_prefix();
	let mut f = String::new();

	let mut start = Ord::clamp(start, 0, 4);
	let mut end = Ord::clamp(end, 0, 4);
	let mut offset = offset;
	let a = out;

	let shift = (start as usize) * 4;
	offset += shift;
	end -= start;
	start = 0;

	match (start, end) {
		(0, 4) => f += &format!("vmovups xmm{a}, [{p}ax + {offset}]\n"),
		(0, 3) => {
			f += &format!("vmovsd xmm{a}, [{p}ax + {offset}]\n");
			f += &format!("vpinsrd xmm{a}, xmm{a}, [{p}ax + {offset} + 8], 0x2\n");
		},
		(0, 2) => f += &format!("vmovsd xmm{a}, [{p}ax + {offset}]\n"),
		(0, 1) => f += &format!("vmovss xmm{a}, [{p}ax + {offset}]\n"),

		_ => panic!(),
	}

	if shift > 0 {
		f += &format!("vpslldq xmm{a}, xmm{a}, {shift}\n");
	}

	f
}

fn store_f32x4(inp: usize, offset: usize, start: i32, end: i32, arch: ArchX86) -> String {
	let p = arch.reg_prefix();
	let mut f = String::new();

	let mut start = Ord::clamp(start, 0, 4);
	let mut end = Ord::clamp(end, 0, 4);
	let mut offset = offset;
	let a = inp;

	if start > 0 {
		let shift = (start as usize) * 4;
		offset += shift;
		end -= start;
		start = 0;

		f += &format!("vpsrldq xmm{a}, xmm{a}, {shift}\n");
	}

	match (start, end) {
		(0, 4) => f += &format!("vmovups [{p}ax + {offset}], xmm{a}\n"),
		(0, 3) => {
			f += &format!("vmovsd [{p}ax + {offset}], xmm{a}\n");
			f += &format!("vpsrldq xmm{a}, xmm{a}, 8\n");
			f += &format!("vmovss [{p}ax + {offset} + 8], xmm{a}\n");
		},
		(0, 2) => f += &format!("vmovsd [{p}ax + {offset}], xmm{a}\n"),
		(0, 1) => f += &format!("vmovss [{p}ax + {offset}], xmm{a}\n"),

		_ => panic!(),
	}
	f
}

fn load_f32x16(start: i32, end: i32, arch: ArchX86) -> String {
	let p = arch.reg_prefix();
	let mut start = start;
	let mut init = false;

	let mut f = String::new();
	if start == 0 && end == 16 {
		f += &format!("vmovups zmm0, [{p}ax]\n");
		init = true;
		start = 16;
	}

	if start == 0 && end >= 8 {
		f += &format!("vmovups ymm0, [{p}ax]\n");
		init = true;
		start = 8;
	}
	if start < 4 && end > start {
		f += &load_f32x4(0, 0, start, end, arch);
		init = true;
		start = 4;
	}
	if !init {
		f += "vxorps xmm0, xmm0, xmm0\n";
	}

	if start == 4 && end >= 8 {
		f += &format!("vinsertf128 ymm0, ymm0, [{p}ax + 16], 0x1\n");
		start = 8;
	}
	if start < 8 && end > start {
		f += &load_f32x4(1, 16, start - 4, end - 4, arch);
		f += "vinsertf128 ymm0, ymm0, xmm1, 0x1\n";
		start = 8;
	}

	if start == 8 && end == 16 {
		f += &format!("vinsertf64x4 zmm0, zmm0, [{p}ax + 32], 0x1\n");
		start = 16;
	}
	if start == 8 && end == 12 {
		f += &format!("vinsertf64x2 zmm0, zmm0, [{p}ax + 32], 0x2\n");
		start = 12;
	}
	if start < 12 && end > start {
		f += &load_f32x4(1, 32, start - 8, end - 8, arch);
		f += "vinsertf64x2 zmm0, zmm0, xmm1, 0x2\n";
		start = 12;
	}

	if start == 12 && end == 16 {
		f += &format!("vinsertf64x2 zmm0, zmm0, [{p}ax + 48], 0x3\n");
		start = 16;
	}
	if start < 16 && end > start {
		f += &load_f32x4(1, 48, start - 12, end - 12, arch);
		f += "vinsertf64x2 zmm0, zmm0, xmm1, 0x3\n";
		start = 12;
	}

	_ = start;

	f
}

fn store_f32x16(start: i32, end: i32, arch: ArchX86) -> String {
	let p = arch.reg_prefix();
	let mut end = end;

	let mut f = String::new();
	if start == 0 && end == 16 {
		f += &format!("vmovups [{p}ax], zmm0\n");
		end = 0;
	}
	if start <= 8 && end == 16 {
		f += "vextractf64x4 ymm1, zmm0, 0x1\n";
		f += &format!("vmovups [{p}ax + 32], ymm1\n");
		end = 8;
	}
	if end > 12 && end > start {
		f += "vextractf64x2 xmm1, zmm0, 0x3\n";
		f += &store_f32x4(1, 48, start - 12, end - 12, arch);
		end = 12;
	}
	if end > 8 && end > start {
		f += "vextractf64x2 xmm1, zmm0, 0x2\n";
		f += &store_f32x4(1, 32, start - 8, end - 8, arch);
		end = 8;
	}

	if start == 0 && end == 8 {
		f += &format!("vmovups [{p}ax], ymm0\n");
		end = 0;
	}
	if end > 4 && end > start {
		f += "vextractf128 xmm1, ymm0, 0x1\n";
		f += &store_f32x4(1, 16, start - 4, end - 4, arch);
		end = 4;
	}
	if end > start {
		f += &store_f32x4(0, 0, start, end, arch);
		end = 0;
	}
	_ = end;

	f
}

fn main() {
	println!("cargo::rustc-check-cfg=cfg(libpulp_const)");
	if let Some(true) = version_check::is_min_version("1.83.0") {
		println!("cargo:rustc-cfg=libpulp_const");
	}

	let arch = if &*env::var("CARGO_CFG_TARGET_ARCH").unwrap() == "x86_64" {
		ArchX86::_64
	} else {
		ArchX86::_32
	};
	let p = arch.reg_prefix();

	let out_dir = env::var_os("OUT_DIR").unwrap();
	let dest_path = Path::new(&out_dir).join("x86_64_asm.rs");

	let mut f = String::new();

	f += "core::arch::global_asm! {\"\n";

	let max = 16;
	let mut wrote_empty = false;

	let mut names = vec![];
	let ver_major = env::var("CARGO_PKG_VERSION_MAJOR").unwrap();
	let ver_minor = env::var("CARGO_PKG_VERSION_MINOR").unwrap();
	let ver_patch = env::var("CARGO_PKG_VERSION_PATCH").unwrap();
	let ver = format!("v{ver_major}_{ver_minor}_{ver_patch}");

	for end in 1..=max {
		for start in 0..max {
			let mut mask = 0u16;
			for i in 0..16 {
				if i >= start && i < end {
					mask |= 1 << (15 - i);
				}
			}

			if mask != 0 || !wrote_empty {
				let ld = format!("libpulp_{ver}_ld_b32s_{mask:0>16b}");
				f += &format!(".global {ld}\n");
				f += &format!("{ld}:\n");
				f += &load_f32x16(start, end, arch);
				f += &format!("jmp {p}cx\n");
				names.push(ld);

				let st = format!("libpulp_{ver}_st_b32s_{mask:0>16b}");
				f += &format!(".global {st}\n");
				f += &format!("{st}:\n");
				f += &store_f32x16(start, end, arch);
				f += &format!("jmp {p}cx\n");
				names.push(st);
			}

			if mask == 0 {
				wrote_empty = true;
			}
		}
	}
	f += "\"}\n";

	f += "extern \"C\" {\n";
	for name in &names {
		f += &format!("#[link_name = \"\\x01{name}\"] fn {name}();\n");
	}
	f += "}\n";

	f += &format!("static LD_ST: [unsafe extern \"C\" fn(); 2 * (({max} + 1) * {max})] = [\n");

	for end in 0..=max {
		for start in 0..max {
			let mut mask = 0u16;
			for i in 0..16 {
				if i >= start && i < end {
					mask |= 1 << (15 - i);
				}
			}

			let ld = format!("libpulp_{ver}_ld_b32s_{mask:0>16b}");
			let st = format!("libpulp_{ver}_st_b32s_{mask:0>16b}");
			f += &format!("{ld}, {st},\n");
		}
	}
	f += "];\n";

	fs::write(&dest_path, &f).unwrap();

	println!("cargo::rerun-if-changed=build.rs");
}
