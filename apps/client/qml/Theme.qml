pragma Singleton
import QtQuick

QtObject {
    property bool isDark: chatController ? chatController.isDarkTheme : true

    // Backgrounds
    readonly property color bgBase: isDark ? "#121418" : "#F4F6F9"
    readonly property color bgSurface: isDark ? "#1A1D24" : "#FFFFFF"
    readonly property color bgElevated: isDark ? "#242832" : "#EAEFF5"
    readonly property color bgBorder: isDark ? "#2E3340" : "#D8DFEB"

    // Primary & Accents
    readonly property color primary: "#3B82F6"       // Vivid Blue
    readonly property color primaryHover: "#2563EB"
    readonly property color accentSuccess: "#10B981"  // Emerald Green (Direct)
    readonly property color accentWarning: "#F59E0B"  // Amber (Relay)
    readonly property color accentInfo: "#0EA5E9"     // Sky Blue (Connecting)
    readonly property color accentOffline: "#6B7280"  // Gray (Offline)
    readonly property color accentDanger: "#EF4444"   // Red

    // Typography
    readonly property color textPrimary: isDark ? "#F9FAFB" : "#111827"
    readonly property color textSecondary: isDark ? "#9CA3AF" : "#4B5563"
    readonly property color textMuted: isDark ? "#6B7280" : "#9CA3AF"

    // Bubble Colors
    readonly property color bubbleOutgoing: "#2563EB"
    readonly property color bubbleIncoming: isDark ? "#252A36" : "#E2E8F0"
    readonly property color bubbleTextOutgoing: "#FFFFFF"
    readonly property color bubbleTextIncoming: isDark ? "#F9FAFB" : "#1E293B"

    // Metrics
    readonly property int radiusSmall: 6
    readonly property int radiusMedium: 10
    readonly property int radiusLarge: 16
    readonly property int radiusPill: 999
}
