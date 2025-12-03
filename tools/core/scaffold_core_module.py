#!/usr/bin/env python3
"""
Prototype helper for scaffolding new core modules.

Usage:
    python tools/core/scaffold_core_module.py --name scheduler --description "Run queue for execution"

The script will create `core/<name>.c` and `core/<name>.h` with starter content if they do not exist.
"""

import argparse
from pathlib import Path
from textwrap import dedent


def create_header(path: Path, module_name: str, description: str) -> None:
    guard = f"HYPERION_{module_name.upper()}_H"
    header_body = dedent(
        f"""
        /**
         * @file {path.name}
         * @brief {description or 'Core module stub.'}
         */
        #ifndef {guard}
        #define {guard}

        #include <stddef.h>

        void {module_name}_init(void);
        void {module_name}_shutdown(void);

        #endif // {guard}
        """
    ).strip() + "\n"
    path.write_text(header_body)


def create_source(path: Path, module_name: str, description: str) -> None:
    source_body = dedent(
        f"""
        /**
         * @file {path.name}
         * @brief {description or 'Core module stub.'}
         */
        #include "{module_name}.h"

        void {module_name}_init(void) {{
            /* TODO: initialize module resources */
        }}

        void {module_name}_shutdown(void) {{
            /* TODO: release module resources */
        }}
        """
    ).strip() + "\n"
    path.write_text(source_body)


def scaffold(output_dir: Path, module_name: str, description: str) -> None:
    normalized_name = module_name.lower()
    header_path = output_dir / f"{normalized_name}.h"
    source_path = output_dir / f"{normalized_name}.c"

    output_dir.mkdir(parents=True, exist_ok=True)

    if header_path.exists():
        print(f"[skip] {header_path} already exists")
    else:
        create_header(header_path, normalized_name, description)
        print(f"[create] {header_path}")

    if source_path.exists():
        print(f"[skip] {source_path} already exists")
    else:
        create_source(source_path, normalized_name, description)
        print(f"[create] {source_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Scaffold a new Hyperion core module.")
    parser.add_argument("--name", required=True, help="Module name (used for file names and symbols)")
    parser.add_argument(
        "--directory",
        default=Path(__file__).resolve().parents[1] / "core",
        type=Path,
        help="Directory where module files will be created (default: repository core/ folder)",
    )
    parser.add_argument("--description", default="Core module stub.", help="Short description for file headers")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    scaffold(args.directory, args.name, args.description)


if __name__ == "__main__":
    main()
