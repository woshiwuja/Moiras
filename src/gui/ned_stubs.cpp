// Stubs for Ned globals that are only used in standalone mode
// but still referenced by ned_util library

#include "lsp/lsp_dashboard.h"

// Provide the global instance
LSPDashboard gLSPDashboard;

// Provide stub implementation for the method
void LSPDashboard::refreshServerInfo() {
    // Empty stub - this is only called from settings UI which we don't use
}
