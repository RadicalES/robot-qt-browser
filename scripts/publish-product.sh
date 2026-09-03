#!/bin/bash
#
# publish-product.sh - write this release's entry into the product catalogue.
#
# The catalogue on cdn.radsys.io is what a customer reads to find out what the
# current version is and where to download it. Nothing about publishing a .deb
# updates it: the per-version manifests are authored, so the catalogue sat at
# 3.6.7 while apt served 3.6.10 - which is the version customers were told to
# install and the version nobody could see.
#
# So this project writes its own entry. It knows what it released, when, and
# what changed; the package repository knows none of that and should not have
# to be told by hand.
#
#   ./scripts/publish-product.sh                     # into the default repo
#   ./scripts/publish-product.sh --repo /path/to/it
#   ./scripts/publish-product.sh --dry-run
#
# What is authored, and where:
#
#   product/product.json   what Robot Browser is - name, summary, links. Rarely
#                          changes.
#   product/version.json   what a release is - description, highlights,
#                          features, notes. Edited when a release is worth
#                          describing differently, which is most of them.
#
# Everything else is computed: the version from VERSION, the date from today,
# the pinned install command and the download URLs from the package names, and
# the product's version list from the directories that actually exist. A list
# appended to drifts; a list rebuilt from the tree cannot.
#
# (C) 2017-2026, Radical Electronic Systems - www.radicalsystems.co.za
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_DIR="${REPO_DIR:-/home/janz/data/package-repository}"
DRY_RUN=false

while [ $# -gt 0 ]; do
    case "$1" in
        --repo)    REPO_DIR="$2"; shift 2 ;;
        --dry-run) DRY_RUN=true; shift ;;
        *) echo "ERROR: unknown option '$1'" >&2; exit 1 ;;
    esac
done

VERSION="$(tr -d '[:space:]' < "$PROJECT_DIR/VERSION")"
PRODUCT_DIR="$REPO_DIR/products/applications/robot-browser"

[ -d "$PRODUCT_DIR" ] || { echo "ERROR: no product tree at $PRODUCT_DIR" >&2; exit 1; }
[ -r "$PROJECT_DIR/product/product.json" ] || { echo "ERROR: product/product.json missing" >&2; exit 1; }
[ -r "$PROJECT_DIR/product/version.json" ] || { echo "ERROR: product/version.json missing" >&2; exit 1; }

echo "Product: robot-browser $VERSION -> $PRODUCT_DIR"

PROJECT_DIR="$PROJECT_DIR" PRODUCT_DIR="$PRODUCT_DIR" VERSION="$VERSION" \
DRY_RUN="$DRY_RUN" python3 - <<'PY'
import json, os, datetime

project = os.environ["PROJECT_DIR"]
product_dir = os.environ["PRODUCT_DIR"]
version = os.environ["VERSION"]
dry_run = os.environ["DRY_RUN"] == "true"

def load(path):
    with open(path) as f:
        return json.load(f)

def write(path, doc):
    if dry_run:
        print("  would write", path)
        return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")
    print("  wrote", path)

# --- this version -----------------------------------------------------------
entry = load(os.path.join(project, "product", "version.json"))
entry["version"] = version
entry["released"] = datetime.date.today().isoformat()
entry["status"] = "stable"
entry["install"] = {
    "method": "apt",
    "package": "robot-browser",
    "command": "sudo apt-get install robot-browser",
    "repository": "https://cdn.radsys.io/debian",
    "pinned": f"sudo apt-get install robot-browser={version}-1~bookworm",
}
entry["downloads"] = [
    {
        "kind": "deb",
        "suite": "bookworm",
        "architecture": arch,
        "url": "https://cdn.radsys.io/debian/pool/main/"
               f"{arch}/robot-browser_{version}-1~bookworm_{arch}.deb",
    }
    for arch in ("arm64", "amd64")
]
write(os.path.join(product_dir, version, "manifest.json"), entry)

# --- the product, rebuilt from the versions that exist ----------------------
#
# Read back rather than appended to, and that includes the entry just written:
# the list then says what is on the CDN rather than what somebody remembered to
# add, and a version removed from the tree disappears from it too.
versions = []
for name in sorted(os.listdir(product_dir)):
    manifest = os.path.join(product_dir, name, "manifest.json")
    if not os.path.isfile(manifest):
        continue
    detail = load(manifest)
    versions.append({
        "version": detail["version"],
        "released": detail["released"],
        "status": detail.get("status", "stable"),
        "path": f"{name}/manifest.json",
    })

def order(v):
    return [int(part) if part.isdigit() else part for part in v["version"].split(".")]

versions.sort(key=order, reverse=True)

product = load(os.path.join(project, "product", "product.json"))
product["latest"] = versions[0]["version"] if versions else version
product["versions"] = versions
write(os.path.join(product_dir, "manifest.json"), product)

print("  latest is", product["latest"])
PY

if [ "$DRY_RUN" = true ]; then
    echo "Dry run - nothing written, nothing generated."
    exit 0
fi

# The catalogue index is generated from the tree, and validated before it is
# trusted: a download that 404s is found here rather than by a customer.
if [ -x "$REPO_DIR/scripts/check-products.sh" ]; then
    echo "=== Validating the product tree ==="
    "$REPO_DIR/scripts/check-products.sh"
fi
if [ -x "$REPO_DIR/scripts/generate-products-index.sh" ]; then
    echo "=== Regenerating products/manifest.json ==="
    "$REPO_DIR/scripts/generate-products-index.sh"
fi

echo ""
echo "Staged. Publish it with, in $REPO_DIR:"
echo "  ./scripts/sync-to-r2.sh --live       # products/ is one of its SYNC_DIRS"
echo ""
echo "Not publish-site.sh: that copies site/public to the bucket root and does"
echo "not carry products/, so the catalogue would stay at the old version while"
echo "the page around it looked freshly published."
