#!/bin/bash
# ESP-IDF Build Script for Peristaltic Pump Tests
# Usage: ./build.sh test_00_blink [build|flash|monitor|clean]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if ESP-IDF is installed
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}ERROR: ESP-IDF not found!${NC}"
    echo ""
    echo "Please install ESP-IDF first:"
    echo "  1. Clone ESP-IDF: git clone --recursive https://github.com/espressif/esp-idf.git ~/esp-idf"
    echo "  2. Install: cd ~/esp-idf && ./install.sh esp32"
    echo "  3. Set up environment: . ~/esp-idf/export.sh"
    echo ""
    echo "Or follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
    exit 1
fi

# Parse arguments
TEST_NAME=${1:-test_00_blink}
ACTION=${2:-build}

# Map test names to source files
declare -A TEST_MAP
TEST_MAP["test_00_blink"]="test/test_00_blink/test_00_blink.c"
TEST_MAP["test_01_buttons"]="test/test_01_buttons/test_01_buttons.c"
TEST_MAP["test_02_encoder"]="test/test_02_encoder/test_02_encoder.c"

# Validate test name
if [ -z "${TEST_MAP[$TEST_NAME]}" ]; then
    echo -e "${RED}ERROR: Unknown test '$TEST_NAME'${NC}"
    echo ""
    echo "Available tests:"
    for test in "${!TEST_MAP[@]}"; do
        echo "  - $test"
    done
    exit 1
fi

SOURCE_FILE="${TEST_MAP[$TEST_NAME]}"

# Check if source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo -e "${RED}ERROR: Source file not found: $SOURCE_FILE${NC}"
    exit 1
fi

echo -e "${GREEN}Building test: $TEST_NAME${NC}"
echo -e "Source: $SOURCE_FILE"
echo ""

# Copy test file to main/main.c
echo -e "${YELLOW}Copying test file to main/main.c...${NC}"
cp "$SOURCE_FILE" main/main.c

# Copy pin definitions
if [ -f "test/common/pin_definitions.h" ]; then
    echo -e "${YELLOW}Copying pin_definitions.h to main/...${NC}"
    cp test/common/pin_definitions.h main/
fi

echo ""

# Execute ESP-IDF command
case $ACTION in
    build)
        echo -e "${GREEN}Building...${NC}"
        idf.py build
        ;;
    flash)
        echo -e "${GREEN}Building and flashing...${NC}"
        idf.py flash
        ;;
    monitor)
        echo -e "${GREEN}Opening serial monitor...${NC}"
        idf.py monitor
        ;;
    clean)
        echo -e "${YELLOW}Cleaning build...${NC}"
        idf.py fullclean
        ;;
    all)
        echo -e "${GREEN}Building, flashing, and monitoring...${NC}"
        idf.py build flash monitor
        ;;
    *)
        echo -e "${RED}ERROR: Unknown action '$ACTION'${NC}"
        echo ""
        echo "Available actions:"
        echo "  build   - Build only"
        echo "  flash   - Build and flash to ESP32"
        echo "  monitor - Open serial monitor"
        echo "  clean   - Clean build artifacts"
        echo "  all     - Build, flash, and monitor"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Done!${NC}"
