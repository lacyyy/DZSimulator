#ifndef COLL_DEBUGGER_H_
#define COLL_DEBUGGER_H_

#include <string>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>

#include "coll/BVH.h"
#include "coll/CollidableWorld-displacement.h"
#include "coll/SweptTrace.h"
#include "gui/GuiState.h"
#include "ren/WideLineRenderer.h"

// Collision procedure visualizer, debug build only 
namespace coll {

class Debugger {
public:

#ifdef NDEBUG
    static constexpr bool IS_ENABLED = false;
#else
    static constexpr bool IS_ENABLED = true;
#endif

    // CAUTION: coll::Debugger is not thread-safe yet!

    // You must call this once map data became invalid/non-existent
    static void Reset();

    // -------------------------------------------------------------------------

    // Debugging a (collision) process requires DebugStart_* to be called before
    // it and DebugFinish_* to be called after it.
    // Some debugged processes happen within other debugged processes.

    // The hierarchy (and examplary call order) of debugging processes is
    // explained by the following declarations:

    static void DebugStart_Trace(const SweptTrace::Info& trace_info);

        static void DebugStart_BroadPhaseLeafHit(const BVH::Leaf& bp_leaf, int32_t bp_leaf_idx);

            // Usable only if hit broad-phase leaf was of type displacement!
            static void DebugStart_DispCollLeafHit(const CDispCollTree& dispcoll, int dispcoll_leaf_idx);
            static void DebugFinish_DispCollLeafHit();

        static void DebugFinish_BroadPhaseLeafHit();

    static void DebugFinish_Trace(const SweptTrace::Results& trace_results);

    // -------------------------------------------------------------------------

    // Were DebugStart_* and DebugFinish_* methods called in an incorrect order?
    static bool DidUsageErrorOccur();
    // If a usage error occurred, returns a description of the error.
    // Returns empty string otherwise.
    static std::string GetUsageErrorDesc();

    // -------------------------------------------------------------------------

    // Draw visualizations
    static void Draw(
        const Magnum::Vector3& cam_pos,
        const Magnum::Vector3& cam_dir_normal,
        const Magnum::Matrix4& view_proj_transformation,
        ren::WideLineRenderer& wide_line_ren,
        gui::GuiState& gui_state
    );

    // Handle/show the collision debugging menu elements
    static void DrawImGuiElements(gui::GuiState& gui_state);

    // -------------------------------------------------------------------------

    // Internal types:
    struct TraceHistoryEntry;
    struct UnfinishedTraceData;
};

} // namespace coll

#endif // COLL_DEBUGGER_H_
