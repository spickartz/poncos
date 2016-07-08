#!/bin/sh
mkdir -p output

    /work/clustsafe --host 192.168.0.217 --user admin -P admin -a energy --reset >/dev/null
    perf stat -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/ snb28/pons_macsnb "${1}" 2>output/temp.txt
    tac output/temp.txt |grep -m 4 . |tac > output/heat-rapl-energy-${threads}.txt
    /work/clustsafe --host 192.168.0.217 --user admin -P admin -a energy --reset |grep "4:" | sed s/"    4: "// >output/${1}-clustsafe-energy.txt

