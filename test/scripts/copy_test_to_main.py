"""
PlatformIO pre-build script to copy test files to main component.
This script runs before compilation and copies the appropriate test file
based on the current environment.
"""

Import("env")
import shutil
import os

# Get the current environment name
env_name = env["PIOENV"]

# Map environment names to test directories
test_mapping = {
    "test_00_blink": "test/test_00_blink/test_00_blink.c",
    "test_01_buttons": "test/test_01_buttons/test_01_buttons.c",
    "test_02_encoder": "test/test_02_encoder/test_02_encoder.c",
}

if env_name in test_mapping:
    test_file = test_mapping[env_name]
    project_dir = env["PROJECT_DIR"]

    # Source and destination paths
    src_path = os.path.join(project_dir, test_file)
    dst_path = os.path.join(project_dir, "main", "main.c")

    # Copy test file to main.c
    if os.path.exists(src_path):
        print(f"Copying {test_file} to main/main.c")
        shutil.copy2(src_path, dst_path)

        # Also copy pin_definitions.h if it exists
        pin_def_src = os.path.join(project_dir, "test", "common", "pin_definitions.h")
        pin_def_dst = os.path.join(project_dir, "main", "pin_definitions.h")
        if os.path.exists(pin_def_src):
            shutil.copy2(pin_def_src, pin_def_dst)
    else:
        print(f"Warning: Test file {test_file} not found!")
