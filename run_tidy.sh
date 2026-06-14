#!/bin/bash
cd ~/DasherCore
for f in $(find src/ -name "*.cpp" -o -name "*.h" | grep -v Thirdparty | sort); do
    clang-tidy -p build "$f" 2>/dev/null
done 2>&1 | grep "warning:" | grep -v "\-Wshadow" | grep -v "ignored-qualifiers" > /tmp/tidy-warnings.txt
echo "=== Warning count by type ==="
sed 's/.*warning: //' /tmp/tidy-warnings.txt | sort | uniq -c | sort -rn
echo ""
echo "=== Unique files with warnings ==="
sed 's/:.*//; s|.*/||' /tmp/tidy-warnings.txt | sort -u
echo ""
echo "=== Full warning list ==="
cat /tmp/tidy-warnings.txt
