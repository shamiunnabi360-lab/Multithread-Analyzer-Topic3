#!/usr/bin/env bash
echo "Date: $(date -u)"
echo "Kernel: $(uname -srm)"
echo "OS: $(. /etc/os-release 2>/dev/null; echo $PRETTY_NAME)"
echo "CPU: $(lscpu 2>/dev/null | sed -n 's/^Model name:[ ]*//p')"
echo "Logical cores: $(nproc)"
lscpu 2>/dev/null | grep -E 'Thread|Core|Socket|L2|L3' | sed 's/^/  /'
echo "RAM: $(free -h 2>/dev/null | awk '/Mem:/{print $2}')"
echo "Compiler: $(gcc --version | head -1)"
echo "Flags: -O2 -Wall -Wextra -std=gnu11 -lpthread"
