#!/bin/bash

SKIP_BUILD=false
ENABLE_COREDUMP=false
USE_GDB=false
USE_VALGRIND=false

for arg in "$@"
do
    if [ "$arg" == "-nobuild" ]; then
        SKIP_BUILD=true
    elif [ "$arg" == "--dump" ]; then
        ENABLE_COREDUMP=true
    elif [ "$arg" == "--gdb" ]; then
        USE_GDB=true
    elif [ "$arg" == "--valgrind" ]; then
        USE_VALGRIND=true
    fi
done

if [ "$SKIP_BUILD" == false ]; then
    ./build.sh
fi

if [ $? -ne 0 ]; then
    exit 1
fi

if [ "$ENABLE_COREDUMP" == true ]; then
    ulimit -c unlimited
    if [ -w /proc/sys/kernel ]; then
        sudo sh -c 'echo "core.%e.%p" > /proc/sys/kernel/core_pattern' 2>/dev/null
    fi
    CORE_PATTERN=$(cat /proc/sys/kernel/core_pattern 2>/dev/null || echo "core")
fi

PLUGIN_PATH="$PWD/build/lib/paraview-5.13/plugins/ViennaPSPluginModule/ViennaPSPluginModule.so"
PARAVIEW_BIN="$HOME/BachelorThesis/paraview/build/bin/paraview"

cat > /tmp/auto_test.py << EOF
from paraview.simple import *
LoadPlugin('$PWD/build/lib/paraview-5.13/plugins/ViennaPSPluginModule/ViennaPSPluginModule.so')
Render()
GetActiveView().ResetCamera()
EOF

if [ "$USE_GDB" == true ]; then
    cat > /tmp/gdb_commands.txt << EOF
set pagination off
set logging file gdb_output.txt
set logging on
run --script=/tmp/auto_test.py
bt full
info locals
info threads
thread apply all bt
quit
EOF
    gdb -x /tmp/gdb_commands.txt "$PARAVIEW_BIN"

elif [ "$USE_VALGRIND" == true ]; then
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             --log-file=valgrind_output.txt \
             --suppressions="$HOME/BachelorThesis/paraview/build/VTK/Utilities/KWIML/vtkkwiml.supp" \
             "$PARAVIEW_BIN" --script=/tmp/auto_test.py

else
    "$PARAVIEW_BIN" --script=/tmp/auto_test.py 2>&1 | tee paraview_stdout.log
    EXIT_CODE=${PIPESTATUS[0]}

    if [ $EXIT_CODE -ne 0 ]; then
        echo "ParaView exited with code: $EXIT_CODE"

        if [ "$ENABLE_COREDUMP" == true ]; then
            CORE_FILES=$(ls -t core* 2>/dev/null | head -1)

            if [ -n "$CORE_FILES" ]; then
                cat > /tmp/gdb_analyze.txt << EOF
set pagination off
set logging file coredump_analysis.txt
set logging on
bt full
info registers
info threads
thread apply all bt full
info locals
info args
quit
EOF

                gdb -batch -x /tmp/gdb_analyze.txt "$PARAVIEW_BIN" "$CORE_FILES"

                head -50 coredump_analysis.txt

                cat > crash_summary.txt << SUMMARY
Date: $(date)
Exit Code: $EXIT_CODE
Core File: $CORE_FILES

$(gdb -batch -ex "bt 10" "$PARAVIEW_BIN" "$CORE_FILES" 2>&1)
SUMMARY

                cat crash_summary.txt

            else
                if command -v coredumpctl &> /dev/null; then
                    coredumpctl list | tail -5
                    LATEST_DUMP=$(coredumpctl list | grep paraview | tail -1 | awk '{print $5}')
                    if [ -n "$LATEST_DUMP" ]; then
                        coredumpctl dump $LATEST_DUMP > systemd_core.dump
                        gdb -batch -x /tmp/gdb_analyze.txt "$PARAVIEW_BIN" systemd_core.dump
                    fi
                fi
            fi
        fi
    fi
fi
