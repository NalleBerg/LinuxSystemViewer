#!/bin/bash
# Test script to reproduce the crash

echo "Testing LSV geek search crash fix..."
echo "Steps to reproduce:"
echo "1. Open LSV"
echo "2. Open a Geek dialog (CPU, Memory, etc.)"
echo "3. Click Search button"
echo "4. Close the geek dialog"
echo "5. Reopen the same geek dialog"  
echo "6. Click Search button again"
echo "Expected: No crash"
echo ""
echo "Starting LSV..."

cd /home/nalle/Dokumenter/c++/lshwgui/build
./LSV