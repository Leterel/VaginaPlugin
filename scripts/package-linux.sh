#!/usr/bin/env bash
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=$(realpath -- "${1:-$project_dir/build-linux}")
output_dir=${2:-$project_dir/dist}
bundle="$build_dir/VST3/Release/VaginaPlugin.vst3"
test -f "$bundle/Contents/x86_64-linux/VaginaPlugin.so" || { echo 'Compiled Linux bundle missing.' >&2; exit 1; }
mkdir -p -- "$output_dir"
output_dir=$(realpath -- "$output_dir")
version=$(sed -nE 's/^project\(VaginaPlugin VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$project_dir/CMakeLists.txt")
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Cannot determine version.' >&2; exit 1; }
archive="VaginaPlugin-$version-linux-x64.tar.gz"
test ! -e "$output_dir/$archive" || { echo 'Archive already exists; select a new output directory.' >&2; exit 1; }
stage=$(mktemp -d "$output_dir/.package-XXXXXXXX")
cleanup() {
  case "$stage" in "$output_dir"/.package-*) rm -rf -- "$stage" ;; *) echo 'Unexpected staging path; preserved.' >&2 ;; esac
}
trap cleanup EXIT
cp -a -- "$bundle" "$stage/VaginaPlugin.vst3"
for item in README.md BUILD.md LICENSE THIRD_PARTY_NOTICES.md licenses; do
  cp -a -- "$project_dir/$item" "$stage/$item"
done
tar -C "$stage" -czf "$output_dir/$archive" VaginaPlugin.vst3 README.md BUILD.md LICENSE THIRD_PARTY_NOTICES.md licenses
(cd -- "$output_dir" && sha256sum "$archive" > "$archive.sha256")
printf '%s\n' "$output_dir/$archive"
