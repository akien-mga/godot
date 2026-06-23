#!/bin/bash -e

# Skia update script for Godot.
#
# Godot vendors only the SkPathOps geometry subset needed by Simplify(), which
# is used to resolve font contour self-intersections (which `embolden` readily
# produces) from TextMesh in scene/resources/3d/primitive_meshes.cpp and from
# msdfgen. The GPU backends, text/font handling, image codecs and serialization
# are not bundled.
#
# The vendored file set is:
#  - the .cpp files listed in modules/skia/SCsub;
#  - the headers those .cpp files actually include.
#
# That header set is derived here with the compiler rather than maintained by
# hand, because a naive scan of #include lines gets it badly wrong: it walks
# into disabled preprocessor blocks and drags in SkCanvas.h, SkBitmap.h,
# SkPaint.h, SkRegion.h, SkStream.h and their dependencies (some 9,000 lines)
# through the `#if DEBUG_DUMP_VERIFY` block in SkPathOpsDebug.cpp, which
# FORCE_RELEASE in SkPathOpsDebug.h hardcodes to 0 in every configuration,
# debug builds included. 0002-trim-debug-dump.patch removes that block.
#
# To add or drop a source file, edit modules/skia/SCsub and re-run this script.

VERSION=chrome/m150
GIT_COMMIT=9c7b2dffb2433f5a0cc2b77f06025a09126807ed

# Headers the closure cannot see because they are reached only from a platform
# branch that does not apply to the machine running this script. Keep this list
# as short as the compiler allows; the include check at the end of the script
# reports what is missing if it is too short.
EXTRA_HEADERS=(
  src/base/SkLeanWindows.h  # Windows only, via SkSemaphore/SkThreadID/SkLog_win.
)

CXX=${CXX:-g++}
target=$(dirname "$(realpath "$0")")
repo_root=$(realpath "$target/../..")
scsub="$repo_root/modules/skia/SCsub"

pushd "$target" > /dev/null
rm -rf include src LICENSE ./*.tar.gz tmp

mkdir tmp && pushd tmp > /dev/null

echo "Updating Skia to $VERSION ($GIT_COMMIT)"
curl -L -o skia.tar.gz "https://github.com/google/skia/archive/$GIT_COMMIT.tar.gz"
tar --strip-components=1 -xf skia.tar.gz
rm skia.tar.gz

# Source files to vendor, read from the build so the two cannot drift apart.
mapfile -t sources < <(sed -n 's/^ *"\(src\/[^"]*\.cpp\)",*$/\1/p' "$scsub")
if [ ${#sources[@]} -eq 0 ]; then
  echo "ERROR: no source files found in $scsub" >&2
  exit 1
fi
echo "Vendoring ${#sources[@]} source files listed in modules/skia/SCsub."

for source in "${sources[@]}"; do
  if [ ! -f "$source" ]; then
    echo "ERROR: $source is listed in modules/skia/SCsub but not in upstream." >&2
    exit 1
  fi
done

# Header closure, with preprocessor conditionals evaluated. -MG keeps a header
# that is genuinely absent upstream from aborting the run; anything outside the
# extracted tree is filtered out below.
echo "Resolving header dependencies with $CXX..."
"$CXX" -std=c++17 -MM -MG -DNDEBUG -DSK_RELEASE -DSK_SUPPORT_GPU=0 \
  -I. -Iinclude "${sources[@]}" 2> /dev/null |
  tr ' \\' '\n\n' | sed 's|^\./||' | grep '\.h$' | sort -u |
  while read -r header; do [ -f "$header" ] && echo "$header"; done > headers.txt

cp LICENSE "$target"
while read -r header; do
  mkdir -p "$target/$(dirname "$header")"
  cp "$header" "$target/$header"
done < <(cat headers.txt <(printf '%s\n' "${EXTRA_HEADERS[@]}" | sed 's/ *#.*//'))

for source in "${sources[@]}"; do
  mkdir -p "$target/$(dirname "$source")"
  cp "$source" "$target/$source"
done

echo "Copied ${#sources[@]} sources and $(wc -l < headers.txt) headers (+${#EXTRA_HEADERS[@]} platform)."

popd > /dev/null
rm -rf tmp

# Godot-specific patches, applied in order. Idempotent.
# Patch paths are relative to this directory, so git apply is run from the repo
# root with --directory: run from here it would resolve them against the repo
# root anyway, silently skip every file, and still exit 0.
echo "Applying patches..."
for patch in patches/*.patch; do
  [ -e "$patch" ] || { echo "  No patches found."; break; }
  apply=(git -C "$repo_root" apply --directory="${target#"$repo_root"/}")
  if "${apply[@]}" --check "$target/$patch" > /dev/null 2>&1; then
    "${apply[@]}" "$target/$patch"
    echo "  $(basename "$patch"): applied"
  elif "${apply[@]}" --reverse --check "$target/$patch" > /dev/null 2>&1; then
    echo "  $(basename "$patch"): already applied"
  else
    echo "  $(basename "$patch"): FAILED to apply" >&2
    exit 1
  fi
done

# Every #include in the vendored tree must resolve to a vendored file. A miss is
# either a header to add to EXTRA_HEADERS, or an include that a patch should
# have removed along with the code using it.
# Skia includes its own headers by their path from the source root, so anything
# without a slash is a vendor branch resolved through some other include path
# (SkLoadUserConfig.h does this for Google3) and is not ours to satisfy.
echo "Checking that all includes resolve..."
grep -rho '#include *"[^"]*/[^"]*\.h"' include src | sed 's/.*"\(.*\)"/\1/' | sort -u |
  while read -r header; do
    [ -f "$header" ] || echo "  Unresolved: $header"
  done

popd > /dev/null

echo "Done. Review 'git status' for files to add or remove, and update the"
echo "version in thirdparty/README.md."
