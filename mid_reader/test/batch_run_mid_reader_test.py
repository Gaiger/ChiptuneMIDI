#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import sys
import time
import os
from pathlib import Path


################################################################################
def main() -> int:
	total_start = time.perf_counter()
	base_dir = Path.cwd()
	if len(sys.argv) >= 2:
		base_dir = Path(sys.argv[1])
	base_dir = base_dir.resolve()

	if not base_dir.is_dir():
		print(f"not a directory: {base_dir}")
		return -1

	script_dir = Path(__file__).resolve().parent
	executable = None
	executable_env = os.environ.get("MID_READER_EXE", "").strip()
	if executable_env:
		env_path = Path(executable_env).expanduser()
		candidates = [env_path / "mid_reader_test"] if env_path.is_dir() else [env_path]
		for candidate in candidates:
			if candidate.is_file():
				executable = candidate.resolve()
				break

	if executable is None:
		candidate = script_dir / "mid_reader_test"
		if candidate.is_file():
			executable = candidate.resolve()

	if executable is None:
		print("executable not found")
		print("set MID_READER_EXE to the executable path or its parent directory")
		print("or place mid_reader_test next to batch_test_mid_reader.py")
		print("example:")
		print("MID_READER_EXE=/path/to/exe_or_dir python3 batch_test_mid_reader.py <midi_dir>")
		return -2

	midi_paths = sorted(
		path for path in base_dir.iterdir()
		if path.is_file() and path.suffix.lower() == ".mid"
	)

	if not midi_paths:
		print(f"no .mid files found in {base_dir}")
		return 0

	print(f"{len(midi_paths)} .mid file(s) found", flush=True)
	print("========================================", flush=True)

	for midi_path in midi_paths:
		print(f"test file ===> {midi_path.name}", flush=True)
		file_start = time.perf_counter()
		completed = subprocess.run([str(executable), str(midi_path)])
		file_elapsed = time.perf_counter() - file_start
		print(f"elapsed = {file_elapsed:.3f}s", flush=True)
		if completed.returncode != 0:
			print(f"stopped on failure: {midi_path.name} (returncode={completed.returncode})", flush=True)
			return completed.returncode

	total_elapsed = time.perf_counter() - total_start
	print("========================================", flush=True)
	print(f"all passed: {len(midi_paths)} file(s)", flush=True)
	print(f"total_elapsed = {total_elapsed:.3f}s", flush=True)
	return 0


################################################################################
if __name__ == "__main__":
	raise SystemExit(main())
