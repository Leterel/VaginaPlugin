"""Verify a real release archive against its tested compiled Linux binary."""
import hashlib
from pathlib import Path
import sys
import tarfile

if len(sys.argv) != 3:
    raise SystemExit("Usage: python tests/check_linux_package.py ARCHIVE COMPILED_SO")
archive, binary = map(Path, sys.argv[1:])
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
assert archive.with_name(archive.name + ".sha256").read_text().strip() == f"{digest}  {archive.name}", "archive checksum mismatch"
with tarfile.open(archive, "r:gz") as package:
    members = package.getmembers()
    names = {member.name for member in members}
    required = {"vajayjay.vst3/Contents/x86_64-linux/vajayjay.so",
                "vajayjay.vst3/Contents/Resources/moduleinfo.json", "README.md",
                "VERIFICATION-0.1.3.md", "LICENSE", "THIRD_PARTY_NOTICES.md"}
    assert required <= names, f"missing entries: {required - names}"
    assert any(name.startswith("licenses/") for name in names), "dependency licenses missing"
    for member in members:
        assert not member.name.startswith(("/", "VaginaPlugin.vst3/")), "unexpected old/absolute path"
        assert ".." not in Path(member.name).parts, "parent path in archive"
        assert "stale-sentinel" not in member.name and ".package-" not in member.name, "stale staging data"
    stream = package.extractfile("vajayjay.vst3/Contents/x86_64-linux/vajayjay.so")
    content = stream.read()
    assert content.startswith(b"\x7fELF"), "not a Linux ELF module"
    assert hashlib.sha256(content).digest() == hashlib.sha256(binary.read_bytes()).digest(), "archive binary differs from tested build"
print("PASS: renamed Linux bundle, exact compiled ELF, notices, safe paths and archive checksum")
