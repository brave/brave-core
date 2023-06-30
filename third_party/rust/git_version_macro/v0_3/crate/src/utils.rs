use std::ffi::OsStr;
use std::path::PathBuf;
use std::process::Command;

/// Remove a trailing newline from a byte string.
fn strip_trailing_newline(mut input: Vec<u8>) -> Vec<u8> {
	if input.len() > 0 && input[input.len() - 1] == b'\n' {
		input.pop();
	}
	input
}

/// Run `git describe` for the current working directory with custom flags to get version information from git.
pub fn describe_cwd<I, S>(args: I) -> std::io::Result<String>
where
	I: IntoIterator<Item = S>,
	S: AsRef<OsStr>,
{
	let cmd = Command::new("git")
		.arg("describe")
		.args(args)
		.output()?;

	let output = verbose_command_error("git describe", cmd)?;
	let output = strip_trailing_newline(output.stdout);

	Ok(String::from_utf8_lossy(&output).to_string())
}

/// Get the git directory for the current working directory.
pub fn git_dir_cwd() -> std::io::Result<PathBuf> {
	// Run git rev-parse --git-dir, and capture standard output.
	let cmd = Command::new("git")
		.args(&["rev-parse", "--git-dir"])
		.output()?;

	let output = verbose_command_error("git rev-parse --git-dir", cmd)?;
	let output = strip_trailing_newline(output.stdout);

	// Parse the output as UTF-8.
	let path = std::str::from_utf8(&output)
		.map_err(|_| std::io::Error::new(std::io::ErrorKind::Other, "invalid UTF-8 in path to .git directory"))?;

	Ok(PathBuf::from(path))
}

#[test]
fn test_git_dir() {
	use std::path::Path;

	assert_eq!(
		git_dir_cwd().unwrap().canonicalize().unwrap(),
		Path::new(env!("CARGO_MANIFEST_DIR")).join("../.git").canonicalize().unwrap()
	);
}

/// Check if a command ran successfully, and if not, return a verbose error.
fn verbose_command_error<C>(command: C, output: std::process::Output) -> std::io::Result<std::process::Output>
where
	C: std::fmt::Display,
{
	// If the command succeeded, just return the output as is.
	if output.status.success() {
		Ok(output)

	// If the command terminated with non-zero exit code, return an error.
	} else if let Some(status) = output.status.code() {
		// Include the first line of stderr in the error message, if it's valid UTF-8 and not empty.
		let message = output.stderr.splitn(2, |c| *c == b'\n').next().unwrap();
		if let Some(message) = String::from_utf8(message.to_vec()).ok().filter(|x| !x.is_empty()) {
			Err(std::io::Error::new(
				std::io::ErrorKind::Other,
				format!("{} failed with status {}: {}", command, status, message),
			))
		} else {
			Err(std::io::Error::new(
				std::io::ErrorKind::Other,
				format!("{} failed with status {}", command, status),
			))
		}

	// The command was killed by a signal.
	} else {
		// Include the signal number on Unix.
		#[cfg(target_family = "unix")]
		{
			use std::os::unix::process::ExitStatusExt;
			let signal = output.status.signal().unwrap();
			Err(std::io::Error::new(
				std::io::ErrorKind::Other,
				format!("{} killed by signal {}", command, signal),
			))
		}
		#[cfg(not(target_family = "unix"))]
		{
			Err(std::io::Error::new(
				std::io::ErrorKind::Other,
				format!("{} killed by signal", command),
			))
		}
	}
}
