#!/usr/bin/env python3
"""Fail when a project-local include violates the layered dependency rules."""

from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')

LAYER_ROOTS = {
    "app": ROOT / "app",
    "services": ROOT / "services",
    "ecual": ROOT / "ecual",
    "bsp": ROOT / "bsp",
    "mcal": ROOT / "mcal",
    "platform": ROOT / "platform",
    "common": ROOT / "common",
    "system": ROOT / "system",
    "startup": ROOT / "startup",
    "config": ROOT / "config",
}

ALLOWED = {
    "app": {"app", "services", "common", "config"},
    "services": {"services", "bsp", "ecual", "common", "config"},
    "ecual": {"ecual", "mcal", "common", "config"},
    "bsp": {"bsp", "mcal", "common", "config"},
    "mcal": {"mcal", "platform", "common", "config"},
    "platform": {"platform", "common", "config"},
    "common": {"common", "config"},
    # Composition root / infrastructure exceptions.
    "system": set(LAYER_ROOTS),
    "startup": {"startup", "common", "config"},
    "config": {"config", "common"},
}

SOURCE_SUFFIXES = {".c", ".h", ".S"}


def owning_layer(path: Path) -> str | None:
    for layer, root in LAYER_ROOTS.items():
        try:
            path.relative_to(root)
            return layer
        except ValueError:
            pass
    return None


def build_header_index() -> dict[str, set[str]]:
    index: dict[str, set[str]] = defaultdict(set)
    for layer, root in LAYER_ROOTS.items():
        if not root.exists():
            continue
        for header in root.rglob("*.h"):
            index[header.name].add(layer)
            index[str(header.relative_to(root))].add(layer)
    return index


def main() -> int:
    header_index = build_header_index()
    errors: list[str] = []

    for source in sorted(ROOT.rglob("*")):
        if not source.is_file() or source.suffix not in SOURCE_SUFFIXES:
            continue
        if "build" in source.parts:
            continue

        source_layer = owning_layer(source)
        if source_layer is None:
            continue

        for line_number, line in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
            match = INCLUDE_RE.match(line)
            if not match:
                continue

            include_name = match.group(1)
            target_layers = header_index.get(include_name)
            if not target_layers:
                # System/standard/external header or a future private header.
                continue

            if target_layers.isdisjoint(ALLOWED[source_layer]):
                target_text = ", ".join(sorted(target_layers))
                errors.append(
                    f"{source.relative_to(ROOT)}:{line_number}: "
                    f"layer '{source_layer}' may not include '{include_name}' "
                    f"from layer(s): {target_text}"
                )

    if errors:
        print("Layer dependency violations:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("Layer dependency check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
