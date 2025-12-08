// EXAMPLE: How to integrate Geek Search with existing AudioGeekDialog
// Add this to audio_tab.cpp in the AudioGeekDialog constructor after line 593:

/*
    // ADD THIS CODE AFTER THE RESCAN BUTTON IS CREATED:
    
    #include "geek_search_integration.h"  // Add at top of file
    
    // In AudioGeekDialog constructor, after rescanBtn is created:
    QPushButton* searchBtn = GeekSearchIntegration::addSearchButtonToGeekDialog(buttonLayout, this);
    
    // That's it! The search button is now available and will open the search dialog
*/

// EXAMPLE: How the progressive search works in practice:
//
// User workflow example:
// 1. Opens Audio Geek Mode → clicks "Search All" 
// 2. Searches for "Intel" → gets 15 results (CPU, GPU, Network, etc.)
// 3. Now "Search in results" option appears
// 4. Searches for "WiFi" in those 15 results → gets 3 WiFi-related Intel results  
// 5. Searches for "6E" in those 3 results → gets 1 final result: "Intel Wi-Fi 6E AX210"
//
// The narrowing continues until only 1 result remains, exactly as requested!

// EXAMPLE: Adding custom data collection for real geek dialogs:
//
// In the real implementation, modify GeekSearchDialog::collectAllGeekData() to:
// 1. Find all open Geek dialogs by searching for QDialog children with specific names
// 2. Call collectFromTreeWidget() on each dialog's QTreeWidget/QTableWidget  
// 3. Use the parent dialog's windowTitle() to determine which tab it represents
//
// This ensures the search works with LIVE data from actual geek dialogs!