#!/bin/bash
CLANG_FORMAT=clang-format
TARGET_FILES=$(find ./src -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \))

echo "start formatting.."
for file in $TARGET_FILES; do
    if [ -f "$file" ]; then
        echo "fxxxxtting $file"
        $CLANG_FORMAT -i -style=file "$file"
    fi
done
echo "done"
