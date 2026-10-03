#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/history/WorkspaceHistoryTracker.hpp>
#include <hyprland/src/state/WorkspaceState.hpp>
#include <hyprland/src/state/workspace/Resolver.hpp>
#include <hyprland/src/workspace/HLWorkspace.hpp>
#include <hyprland/src/workspace/RegularWorkspace.hpp>

// Numeric workspace ids from Hyprland before Workspace::WorkspaceID became a variant.
using WORKSPACEID = int64_t;
inline constexpr WORKSPACEID WORKSPACE_INVALID     = -1;
inline constexpr WORKSPACEID WORKSPACE_NOT_CHANGED = -101;

namespace hymission {

struct SWorkspaceIDName {
    WORKSPACEID id        = WORKSPACE_INVALID;
    std::string name;
    bool        isAutoIDd = false;
};

inline WORKSPACEID workspaceNumericId(const PHLWORKSPACE& workspace) {
    if (!workspace)
        return WORKSPACE_INVALID;

    if (const auto id = workspace->numberedID())
        return static_cast<WORKSPACEID>(*id);

    return WORKSPACE_INVALID;
}

inline WORKSPACEID workspaceNumericId(const PHLWORKSPACEREF& workspace) {
    return workspaceNumericId(workspace.lock());
}

inline bool workspaceIsPersistent(const PHLWORKSPACE& workspace) {
    const auto* regular = workspace ? dynamic_cast<const Workspace::CRegularWorkspace*>(workspace.get()) : nullptr;
    return regular && regular->isPersistent();
}

inline void setWorkspacePersistent(const PHLWORKSPACE& workspace, bool persistent) {
    if (auto* regular = workspace ? dynamic_cast<Workspace::CRegularWorkspace*>(workspace.get()) : nullptr)
        regular->setPersistent(persistent);
}

inline PHLWORKSPACE findNumberedWorkspace(WORKSPACEID id) {
    if (id <= 0 || static_cast<uint64_t>(id) > UINT32_MAX)
        return nullptr;

    return State::workspaceState()
        ->query()
        .numbered(Workspace::SWorkspaceNumberedID{static_cast<Workspace::WorkspaceIDContainer>(id)})
        .run();
}

inline PHLWORKSPACE createNumberedWorkspace(WORKSPACEID id, const PHLMONITOR& monitor, const std::string& name = {}, bool isEmpty = true) {
    if (!monitor || id <= 0 || static_cast<uint64_t>(id) > UINT32_MAX)
        return nullptr;

    return State::workspaceState()->createNumbered(Workspace::SWorkspaceNumberedID{static_cast<Workspace::WorkspaceIDContainer>(id)}, monitor, name, isEmpty);
}

inline SWorkspaceIDName getWorkspaceIDNameFromString(const std::string& input, std::optional<PHLMONITOR> monitor = std::nullopt) {
    SWorkspaceIDName out;
    const auto       target = State::Workspace::resolver()->getWorkspaceTargetFromString(input, monitor);
    if (!target.id)
        return out;

    if (const auto* numbered = std::get_if<Workspace::SWorkspaceNumberedID>(&*target.id))
        out.id = static_cast<WORKSPACEID>(numbered->value);

    out.name = !target.displayName.empty() ? target.displayName : target.address;
    return out;
}

inline SWorkspaceIDName previousWorkspaceIDName(const PHLWORKSPACE& workspace, const PHLMONITOR& restrictMonitor = nullptr) {
    SWorkspaceIDName out;
    if (!workspace)
        return out;

    const auto entry = restrictMonitor ? Desktop::History::workspaceTracker()->previousWorkspace(workspace, restrictMonitor) :
                                         Desktop::History::workspaceTracker()->previousWorkspace(workspace);
    if (const auto live = entry.workspace.lock()) {
        out.id   = workspaceNumericId(live);
        out.name = live->displayName();
        return out;
    }

    if (entry.target.id) {
        if (const auto* numbered = std::get_if<Workspace::SWorkspaceNumberedID>(&*entry.target.id))
            out.id = static_cast<WORKSPACEID>(numbered->value);
        out.name = entry.target.displayName;
    }

    return out;
}

} // namespace hymission
