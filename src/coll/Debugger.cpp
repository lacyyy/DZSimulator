#include "coll/Debugger.h"

#include <cmath>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include "coll/CollidableWorld.h"
#include "coll/CollidableWorld_Impl.h"
#include "coll/CollidableWorld-displacement.h"
#include "coll/SweptTrace.h"
#include "csgo_parsing/BspMap.h"
#include "GlobalVars.h"
#include "ren/WideLineRenderer.h"

using namespace Magnum;
using namespace coll;

// -----------------------------------------------------------------------------

static std::string usage_error_msg = "";
bool        Debugger::DidUsageErrorOccur() { return !usage_error_msg.empty(); }
std::string Debugger::GetUsageErrorDesc()  { return usage_error_msg; }

static void GenerateUsageErrorMsg(std::string_view msg) {
    if (Debugger::DidUsageErrorOccur())
        return; // We want to keep the first error msg that ever appeared
    usage_error_msg = msg;
    Error{} << "[ERROR] [coll::Debugger] Usage Error:" << usage_error_msg.c_str();
}

// -----------------------------------------------------------------------------

struct Debugger::TraceHistoryEntry
{
    SweptTrace::Info    trace_info;
    SweptTrace::Results trace_results;

    struct BroadPhaseLeafHit {
        int32_t bvh_leaf_idx;
        BVH::Leaf::Type bvh_leaf_type;

        // AABB of referenced map object, but slightly bloated.
        Magnum::Vector3 mins;
        Magnum::Vector3 maxs;


        struct TypeSpecificData_Brush {
            uint32_t brush_idx; // idx into BspMap.brushes
        };

        struct TypeSpecificData_Displacement {
            uint32_t disp_coll_idx; // idx into CDispCollTree array

            struct DispCollLeaf {
                int dispcoll_leaf_idx; // idx into CDispCollTree.m_leaves

                // Exact AABB
                Magnum::Vector3 mins;
                Magnum::Vector3 maxs;
            };
            std::vector<DispCollLeaf> disp_coll_leaf_hits; // In chronological order
        };

        struct TypeSpecificData_FuncBrush {
            uint32_t funcbrush_idx; // idx into BspMap.entities_func_brush
        };

        struct TypeSpecificData_StaticProp {
            uint32_t sprop_idx; // idx into BspMap.static_props
        };

        struct TypeSpecificData_DynamicProp {
            uint32_t dprop_idx; // idx into BspMap.relevant_dynamic_props
        };

        // Data that's specific to the leaf's type
        std::variant<
            TypeSpecificData_Brush,        // Used type if bvh_leaf_type == Brush
            TypeSpecificData_Displacement, // Used type if bvh_leaf_type == Displacement
            TypeSpecificData_FuncBrush,    // Used type if bvh_leaf_type == FuncBrush
            TypeSpecificData_StaticProp,   // Used type if bvh_leaf_type == StaticProp
            TypeSpecificData_DynamicProp   // Used type if bvh_leaf_type == DynamicProp
        > data;
    };
    std::vector<BroadPhaseLeafHit> broad_phase_leaf_hits; // In chronological order
};

const size_t MAX_TRACE_HISTORY_LEN = 50; // Must be 1 or greater
static std::deque<Debugger::TraceHistoryEntry> trace_history;

// -----------------------------------------------------------------------------

// Temporary, incomplete data of a trace that was started but not finished yet
struct Debugger::UnfinishedTraceData {
    using BroadPhaseLeafHit = TraceHistoryEntry::BroadPhaseLeafHit;
    using DispCollLeaf      = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Displacement::DispCollLeaf;

    SweptTrace::Info tr_info;

    // Has value if broad-phase leaf hit was started but not finished yet.
    std::optional<BroadPhaseLeafHit> unfinished_broad_phase_leaf_hit = std::nullopt;

        // ONLY ALLOWED if we're inside an unfinished broad phase leaf hit of type displacement!
        // Has value if dispcoll leaf hit was started but not finished yet.
        std::optional<DispCollLeaf> unfinished_dispcoll_leaf_hit = std::nullopt;


    // In chronological order
    std::vector<BroadPhaseLeafHit> finished_broad_phase_leaf_hits;

    std::optional<SweptTrace::Results> tr_results = std::nullopt; // Has no value until trace is finished
};

// Has no value if previous trace was finished
static std::optional<Debugger::UnfinishedTraceData> unfinished_trace = std::nullopt;


// -----------------------------------------------------------------------------

struct DrawState {
    int64_t selected_broad_phase_leaf_hit_idx = -1;

    // If selected broad-phase leaf hit is of type Displacement, ...
    bool show_dispcoll_leaf_hits = true;
};
static DrawState draw_state;

// -----------------------------------------------------------------------------

void Debugger::Reset()
{
    if (!Debugger::IS_ENABLED) return;
    trace_history.clear();
    unfinished_trace.reset();
    usage_error_msg = "";
    draw_state = {};
}

void Debugger::DebugStart_Trace(const SweptTrace::Info& trace_info)
{
    if (!Debugger::IS_ENABLED) return;
    if (DidUsageErrorOccur()) return;

    // Previous trace must be finished
    if (unfinished_trace) {
        GenerateUsageErrorMsg("DebugStart_Trace(): The previous trace was not "
            "finished through calling the necessary DebugFinish_*() methods!");
        return;
    }

    // Create unfinished_trace object
    unfinished_trace = UnfinishedTraceData{ .tr_info = trace_info };
}

void Debugger::DebugStart_BroadPhaseLeafHit(const BVH::Leaf& leaf, int32_t bp_leaf_idx)
{
    if (!Debugger::IS_ENABLED) return;
    if (DidUsageErrorOccur()) return;

    // Previous trace must be UNFINISHED
    if (!unfinished_trace) {
        GenerateUsageErrorMsg("DebugStart_BroadPhaseLeafHit(): The previous "
            "trace was not started through calling DebugStart_Trace()!");
        return;
    }

    // Previous trace's previous broad-phase leaf hit must be FINISHED
    if (unfinished_trace->unfinished_broad_phase_leaf_hit) {
        GenerateUsageErrorMsg("DebugStart_BroadPhaseLeafHit(): The previous "
            "trace's broad-phase leaf hit was not finished through calling "
            "DebugFinish_BroadPhaseLeafHit()!");
        return;
    }

    // Create unfinished broad-phase leaf hit object
    std::variant<
        TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Brush,
        TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Displacement,
        TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_FuncBrush,
        TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_StaticProp,
        TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_DynamicProp
    > data; // Data that's specific to the leaf's type

    switch (leaf.type) {
        case BVH::Leaf::Type::Brush:
            data = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Brush{
                .brush_idx = leaf.brush_idx
            };
            break;
        case BVH::Leaf::Type::Displacement:
            data = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Displacement{
                .disp_coll_idx = leaf.disp_coll_idx
            };
            break;
        case BVH::Leaf::Type::StaticProp:
            data = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_StaticProp{
                .sprop_idx = leaf.sprop_idx
            };
            break;
        case BVH::Leaf::Type::DynamicProp:
            data = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_DynamicProp{
                .dprop_idx = leaf.dprop_idx
            };
            break;
        case BVH::Leaf::Type::FuncBrush:
            data = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_FuncBrush{
                .funcbrush_idx = leaf.funcbrush_idx
            };
            break;
    }

    unfinished_trace->unfinished_broad_phase_leaf_hit = {
        .bvh_leaf_idx = bp_leaf_idx,
        .bvh_leaf_type = leaf.type,
        .mins = leaf.mins,
        .maxs = leaf.maxs,
        .data = data
    };
}

void Debugger::DebugStart_DispCollLeafHit(const CDispCollTree& dispcoll,
    int dispcoll_leaf_idx)
{
    if (!Debugger::IS_ENABLED) return;
    if (DidUsageErrorOccur()) return;

    // Previous trace must be UNFINISHED
    if (!unfinished_trace) {
        GenerateUsageErrorMsg("DebugStart_DispCollLeafHit(): The previous "
            "trace was not started through calling DebugStart_Trace()!");
        return;
    }

    // Previous trace's previous broad-phase leaf hit must be UNFINISHED
    if (!unfinished_trace->unfinished_broad_phase_leaf_hit) {
        GenerateUsageErrorMsg("DebugStart_DispCollLeafHit(): The previous "
            "trace's broad-phase leaf hit was not started through calling "
            "DebugStart_BroadPhaseLeafHit()!");
        return;
    }

    // Previous trace's previous dispcoll leaf hit must be FINISHED
    if (unfinished_trace->unfinished_dispcoll_leaf_hit) {
        GenerateUsageErrorMsg("DebugStart_DispCollLeafHit(): The previous "
            "dispcoll leaf hit was not finished through calling "
            "DebugFinish_DispCollLeafHit()!");
        return;
    }

    // Determine dispcoll leaf's AABB
    Vector3 mins = { +HUGE_VALF, +HUGE_VALF, +HUGE_VALF };
    Vector3 maxs = { -HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
    for (int i = 0; i < 2; i++) {
        int iTri = dispcoll.m_leaves[dispcoll_leaf_idx].m_tris[i];
        const CDispCollTri& pTri = dispcoll.m_aTris[iTri];
        for (int j = 0; j < 3; j++) {
            const Vector3& vert = dispcoll.m_aVerts[pTri.GetVert(j)];
            for (int axis = 0; axis < 3; axis++) {
                if (vert[axis] < mins[axis]) mins[axis] = vert[axis];
                if (vert[axis] > maxs[axis]) maxs[axis] = vert[axis];
            }
        }
    }

    using DispCollLeaf = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Displacement::DispCollLeaf;
    unfinished_trace->unfinished_dispcoll_leaf_hit = DispCollLeaf{
        .dispcoll_leaf_idx = dispcoll_leaf_idx,
        .mins = mins,
        .maxs = maxs
    };
}

void Debugger::DebugFinish_DispCollLeafHit()
{
    if (!Debugger::IS_ENABLED) return;
    if (DidUsageErrorOccur()) return;

    // Previous trace must be UNFINISHED
    if (!unfinished_trace) {
        GenerateUsageErrorMsg("DebugFinish_DispCollLeafHit(): The previous "
            "trace was not started through calling DebugStart_Trace()!");
        return;
    }

    // Previous trace's previous broad-phase leaf hit must be UNFINISHED
    if (!unfinished_trace->unfinished_broad_phase_leaf_hit) {
        GenerateUsageErrorMsg("DebugFinish_DispCollLeafHit(): The previous "
            "trace's broad-phase leaf hit was not started through calling "
            "DebugStart_BroadPhaseLeafHit()!");
        return;
    }

    // Previous trace's previous broad-phase leaf hit must be of type DISPLACEMENT
    if (unfinished_trace->unfinished_broad_phase_leaf_hit->bvh_leaf_type != BVH::Leaf::Type::Displacement) {
        GenerateUsageErrorMsg("DebugFinish_DispCollLeafHit(): This method must "
            "be called during a broad-phase leaf hit of type displacement!");
        return;
    }

    // Previous trace's previous dispcoll leaf hit must be UNFINISHED
    if (!unfinished_trace->unfinished_dispcoll_leaf_hit) {
        GenerateUsageErrorMsg("DebugFinish_DispCollLeafHit(): The previous "
            "dispcoll leaf hit was not started through calling "
            "DebugStart_DispCollLeafHit()!");
        return;
    }

    using DisplacementData = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Displacement;
    DisplacementData& data =
        std::get<DisplacementData>(unfinished_trace->unfinished_broad_phase_leaf_hit->data);

    // Add to dispcoll leaf hit list
    data.disp_coll_leaf_hits.push_back(
        std::move(*(unfinished_trace->unfinished_dispcoll_leaf_hit))
    );
    // Delete unfinished dispcoll leaf hit object
    unfinished_trace->unfinished_dispcoll_leaf_hit.reset();
}

void Debugger::DebugFinish_BroadPhaseLeafHit()
{
    if (!Debugger::IS_ENABLED) return;
    if (DidUsageErrorOccur()) return;

    // Previous trace must be UNFINISHED
    if (!unfinished_trace) {
        GenerateUsageErrorMsg("DebugFinish_BroadPhaseLeafHit(): The previous "
            "trace was not started through calling DebugStart_Trace()!");
        return;
    }

    // Previous trace's previous broad-phase leaf hit must be UNFINISHED
    if (!unfinished_trace->unfinished_broad_phase_leaf_hit) {
        GenerateUsageErrorMsg("DebugFinish_BroadPhaseLeafHit(): The previous "
            "trace's broad-phase leaf hit was not started through calling "
            "DebugStart_BroadPhaseLeafHit()!");
        return;
    }

    // Previous trace's previous dispcoll leaf hit must be FINISHED
    if (unfinished_trace->unfinished_dispcoll_leaf_hit) {
        GenerateUsageErrorMsg("DebugFinish_BroadPhaseLeafHit(): The previous "
            "trace's dispcoll leaf hit was not finished through calling "
            "DebugFinish_DispCollLeafHit()!");
        return;
    }

    // Add to broad-phase leaf hit list
    unfinished_trace->finished_broad_phase_leaf_hits.push_back(
        std::move(*(unfinished_trace->unfinished_broad_phase_leaf_hit))
    );
    // Delete unfinished broad-phase leaf hit object
    unfinished_trace->unfinished_broad_phase_leaf_hit.reset();
}

void Debugger::DebugFinish_Trace(const SweptTrace::Results& trace_results)
{
    if (!Debugger::IS_ENABLED) return;
    if (DidUsageErrorOccur()) return;

    // Previous trace must be UNFINISHED
    if (!unfinished_trace) {
        GenerateUsageErrorMsg("DebugFinish_Trace(): The previous trace was not "
            "started through calling DebugStart_Trace()!");
        return;
    }

    // Previous trace's previous broad-phase leaf hit must be FINISHED
    if (unfinished_trace->unfinished_broad_phase_leaf_hit) {
        GenerateUsageErrorMsg("DebugFinish_Trace(): The previous trace's broad-"
            "phase leaf hit was not finished through calling "
            "DebugFinish_BroadPhaseLeafHit()!");
        return;
    }

    // Previous trace's previous dispcoll leaf hit must be FINISHED
    if (unfinished_trace->unfinished_dispcoll_leaf_hit) {
        GenerateUsageErrorMsg("DebugFinish_Trace(): The previous trace's "
            "dispcoll leaf hit was not finished through calling "
            "DebugFinish_DispCollLeafHit()!");
        return;
    }

    // Complete the unfinished trace
    unfinished_trace->tr_results = trace_results;

    // Create history entry
    TraceHistoryEntry new_entry = {
        .trace_info = unfinished_trace->tr_info,
        .trace_results = *(unfinished_trace->tr_results),
        .broad_phase_leaf_hits = std::move(unfinished_trace->finished_broad_phase_leaf_hits)
    };
    // Delete unfinished_trace object
    unfinished_trace.reset();

    if (trace_history.size() >= MAX_TRACE_HISTORY_LEN)
        trace_history.pop_front();
    trace_history.push_back(std::move(new_entry));

    // Reset draw state
    draw_state = {};
}

void Debugger::Draw(
    const Vector3& cam_pos,
    const Vector3& cam_dir_normal,
    const Matrix4& view_proj_transformation,
    ren::WideLineRenderer& wide_line_ren,
    gui::GuiState& gui_state)
{
    if (!Debugger::IS_ENABLED)
        return;

    const Color3 GRAY   = { 0.6f, 0.6f, 0.6f };
    const Color3 RED    = { 0.9f, 0.0f, 0.0f };
    const Color3 GREEN  = { 0.0f, 0.9f, 0.0f };
    const Color3 AQUA   = { 0.2f, 0.8f, 1.0f };

    for (const TraceHistoryEntry& entry : trace_history)
    {
        const SweptTrace::Info&    tr_info    = entry.trace_info;
        const SweptTrace::Results& tr_results = entry.trace_results;
        Vector3 reached_pos = tr_info.startpos + tr_results.fraction * tr_info.delta;

        // Success/failure color
        Color3 status_color = tr_results.fraction == 1.0f ? RED : GREEN;

        // -----------------------------------------------

        // Draw AABB at trace START
        if (tr_results.fraction != 0.0f) {
            // Drawing a box right around the camera is irritating
            bool is_cam_inside_start_aabb =
                Math::abs(cam_pos.x() - tr_info.startpos.x()) < tr_info.extents.x() &&
                Math::abs(cam_pos.y() - tr_info.startpos.y()) < tr_info.extents.y() &&
                Math::abs(cam_pos.z() - tr_info.startpos.z()) < tr_info.extents.z();
            if (!is_cam_inside_start_aabb) {
                wide_line_ren.DrawAABB(GRAY,
                    tr_info.startpos,
                    2 * tr_info.extents,
                    view_proj_transformation, cam_pos, cam_dir_normal
                );
            }
        }

        // Draw AABB at trace END
        if (tr_results.fraction != 1.0f) {
            wide_line_ren.DrawAABB(GRAY,
                tr_info.startpos + tr_info.delta,
                2 * tr_info.extents,
                view_proj_transformation, cam_pos, cam_dir_normal
            );
        }

        // Draw AABB at collision point
        wide_line_ren.DrawAABB(status_color,
            reached_pos,
            2 * tr_info.extents,
            view_proj_transformation, cam_pos, cam_dir_normal
        );

        // -----------------------------------------------

        // Draw line from START to COLLISION POINT
        wide_line_ren.DrawLine(status_color,
            tr_info.startpos,
            reached_pos,
            view_proj_transformation, cam_pos, cam_dir_normal
        );

        // Draw line from COLLISION POINT to END
        wide_line_ren.DrawLine(GRAY,
            reached_pos,
            tr_info.startpos + tr_info.delta,
            view_proj_transformation, cam_pos, cam_dir_normal
        );

        // -----------------------------------------------

        // Draw plane normal at impact point
        if (tr_results.fraction != 1.0f && !tr_results.startsolid) {
            wide_line_ren.DrawDirectionIndicator(AQUA,
                reached_pos,
                tr_results.plane_normal,
                view_proj_transformation, cam_pos, cam_dir_normal
            );
        }

        // -----------------------------------------------
        // -----------------------------------------------
        // -----------------------------------------------

        // Draw AABB of hit broad-phase leaves
        const Color3 bp_color = Color3{ 1.0f, 0.0f, 0.0f };
        for (size_t i = 0; i < entry.broad_phase_leaf_hits.size(); i++) {
            const auto& bp_leaf_hit = entry.broad_phase_leaf_hits[i];

            bool draw = (draw_state.selected_broad_phase_leaf_hit_idx == i)
                || (draw_state.selected_broad_phase_leaf_hit_idx == -1);
            if (draw) {
                wide_line_ren.DrawAABB(bp_color,
                    0.5f * (bp_leaf_hit.mins + bp_leaf_hit.maxs),
                    bp_leaf_hit.maxs - bp_leaf_hit.mins,
                    view_proj_transformation, cam_pos, cam_dir_normal);
            }
        }

        // Draw displacement specific information
        if (draw_state.show_dispcoll_leaf_hits) {
            const Color3 dispcoll_color = Color3{ 0.5f, 0.5f, 0.5f };
            if (draw_state.selected_broad_phase_leaf_hit_idx >= 0 &&
                draw_state.selected_broad_phase_leaf_hit_idx < entry.broad_phase_leaf_hits.size()) {
                const auto& bp_leaf_hit = entry.broad_phase_leaf_hits[draw_state.selected_broad_phase_leaf_hit_idx];

                if (bp_leaf_hit.bvh_leaf_type == BVH::Leaf::Displacement) {
                    using TypeSpecificData_Displacement = TraceHistoryEntry::BroadPhaseLeafHit::TypeSpecificData_Displacement;
                    using DispCollLeaf = TypeSpecificData_Displacement::DispCollLeaf;
                    const std::vector<DispCollLeaf>& dispcoll_leaf_hits =
                        std::get<TypeSpecificData_Displacement>(bp_leaf_hit.data).disp_coll_leaf_hits;

                    for (size_t dispcoll_leaf_hit_idx = 0; dispcoll_leaf_hit_idx < dispcoll_leaf_hits.size(); dispcoll_leaf_hit_idx++) {
                        const DispCollLeaf& dispcoll_leaf = dispcoll_leaf_hits[dispcoll_leaf_hit_idx];

                        wide_line_ren.DrawAABB(dispcoll_color,
                            0.5f * (dispcoll_leaf.mins + dispcoll_leaf.maxs),
                            dispcoll_leaf.maxs - dispcoll_leaf.mins,
                            view_proj_transformation, cam_pos, cam_dir_normal);
                    }
                }
            }
        }
    }

    using DispInfo = csgo_parsing::BspMap::DispInfo;

    if (gui_state.coll_debug.IN_showDispsForHullColl) {
        const Color3 color = Color3{ 1.0f, 0.0f, 1.0f };
        if (g_coll_world && g_coll_world->pImpl->hull_disp_coll_trees) {
            for (const CDispCollTree& dispcoll : *g_coll_world->pImpl->hull_disp_coll_trees) {
                if (dispcoll.CheckFlags(DispInfo::FLAG_NO_HULL_COLL))
                    continue;

                wide_line_ren.DrawAABB(color,
                    0.5f * (dispcoll.m_mins + dispcoll.m_maxs),
                    dispcoll.m_maxs - dispcoll.m_mins,
                    view_proj_transformation, cam_pos, cam_dir_normal);
            }
        }
    }

    // Note: This draws after the other displacement visualizations in order to
    //       overdraw them. The following visualization is more important.
    if (gui_state.coll_debug.IN_showDispsWithCollCache && g_coll_world) {
        const Color3 color = Color3{ 1.0f, 1.0f, 0.0f };
        if (g_coll_world && g_coll_world->pImpl->hull_disp_coll_trees) {
            for (const CDispCollTree& dispcoll : *g_coll_world->pImpl->hull_disp_coll_trees) {
                if (!dispcoll.IsCacheGenerated())
                    continue;

                wide_line_ren.DrawAABB(color,
                    0.5f * (dispcoll.m_mins + dispcoll.m_maxs),
                    dispcoll.m_maxs - dispcoll.m_mins,
                    view_proj_transformation, cam_pos, cam_dir_normal);
            }
        }
    }
}

void Debugger::DrawImGuiElements(gui::GuiState& gui_state)
{
    if (!Debugger::IS_ENABLED)
        return;

    ImGui::Checkbox("Show displacements used for hull traces",
        &gui_state.coll_debug.IN_showDispsForHullColl);
    ImGui::Checkbox("Show displacements with a generated collision cache",
        &gui_state.coll_debug.IN_showDispsWithCollCache);

    ImGui::Separator();

    if (trace_history.empty()) {
        ImGui::Text("No trace data to show.");
        return;
    }

    const TraceHistoryEntry& entry = trace_history.back(); // Newest entry
    const SweptTrace::Info&    tr_info    = entry.trace_info;
    const SweptTrace::Results& tr_results = entry.trace_results;

    ImGui::Text("tr.startpos = (%.2F, %.2F, %.2F)",
        tr_info.startpos.x(),
        tr_info.startpos.y(),
        tr_info.startpos.z());
    ImGui::Text("tr.delta = (%.2F, %.2F, %.2F)",
        tr_info.delta.x(),
        tr_info.delta.y(),
        tr_info.delta.z());
    ImGui::Text("tr.invdelta = (%.5F, %.5F, %.5F)",
        tr_info.invdelta.x(),
        tr_info.invdelta.y(),
        tr_info.invdelta.z());

    ImGui::Text("tr.extents = (%.2F, %.2F, %.2F)",
        tr_info.extents.x(),
        tr_info.extents.y(),
        tr_info.extents.z());
    ImGui::SameLine();
    ImGui::Text(tr_info.isray ? "tr.isray = true" : "tr.isray = false");

    ImGui::Text(tr_results.startsolid ? "tr.startsolid = true" : "tr.startsolid = false");
    ImGui::Text(tr_results.allsolid ? "tr.allsolid = true" : "tr.allsolid = false");
    ImGui::Text("tr.fraction = %.4F", tr_results.fraction);
    ImGui::Text("tr.plane_normal = (%.3F, %.3F, %.3F)",
        tr_results.plane_normal.x(),
        tr_results.plane_normal.y(),
        tr_results.plane_normal.z());
    ImGui::Text("tr.surface = %i", tr_results.surface);

    ImGui::Separator();

    // Show hierarchical trace debug info

    if (DidUsageErrorOccur()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "Usage error: %s", usage_error_msg.c_str());
        return;
    }

    using BroadPhaseLeafHit = TraceHistoryEntry::BroadPhaseLeafHit;
    int64_t& selected_idx = draw_state.selected_broad_phase_leaf_hit_idx;
    for (size_t i = 0; i < entry.broad_phase_leaf_hits.size(); i++) {
        const BroadPhaseLeafHit& bp_leaf_hit = entry.broad_phase_leaf_hits[i];

        std::string label = "Hit #" + std::to_string(i) + ": Leaf #";
        label += std::to_string(bp_leaf_hit.bvh_leaf_idx) + ", ";

        switch (bp_leaf_hit.bvh_leaf_type) {
        case BVH::Leaf::Brush:
            label += "Brush #";
            label += std::to_string(std::get<BroadPhaseLeafHit::TypeSpecificData_Brush>(bp_leaf_hit.data).brush_idx);
            break;
        case BVH::Leaf::Displacement:
            label += "Displacement #";
            label += std::to_string(std::get<BroadPhaseLeafHit::TypeSpecificData_Displacement>(bp_leaf_hit.data).disp_coll_idx);
            break;
        case BVH::Leaf::FuncBrush:
            label += "FuncBrush #";
            label += std::to_string(std::get<BroadPhaseLeafHit::TypeSpecificData_FuncBrush>(bp_leaf_hit.data).funcbrush_idx);
            break;
        case BVH::Leaf::StaticProp:
            label += "StaticProp #";
            label += std::to_string(std::get<BroadPhaseLeafHit::TypeSpecificData_StaticProp>(bp_leaf_hit.data).sprop_idx);
            break;
        case BVH::Leaf::DynamicProp:
            label += "DynamicProp #";
            label += std::to_string(std::get<BroadPhaseLeafHit::TypeSpecificData_DynamicProp>(bp_leaf_hit.data).dprop_idx);
            break;
        }

        ImGui::Indent();
        if (ImGui::Selectable(label.c_str(), selected_idx == i)) {
            int64_t old_selected_idx = selected_idx;
            selected_idx = i;
            bool new_selection = old_selected_idx != selected_idx;

            if (new_selection) {
                // Reset some draw state if new broad-phase hit was selected
                //draw_state.show_dispcoll_leaf_hits = true;
            }
        }

        // Draw displacement specific debug elements
        if (selected_idx == i && BVH::Leaf::Displacement == bp_leaf_hit.bvh_leaf_type) {
            ImGui::Indent();
            ImGui::Checkbox("Show hit DispColl leaves", &draw_state.show_dispcoll_leaf_hits);

            if (draw_state.show_dispcoll_leaf_hits) {
                using DispCollLeaf = BroadPhaseLeafHit::TypeSpecificData_Displacement::DispCollLeaf;
                const std::vector<DispCollLeaf>& dispcoll_leaf_hits =
                    std::get<BroadPhaseLeafHit::TypeSpecificData_Displacement>(bp_leaf_hit.data).disp_coll_leaf_hits;

                for (size_t dispcoll_leaf_hit_idx = 0; dispcoll_leaf_hit_idx < dispcoll_leaf_hits.size(); dispcoll_leaf_hit_idx++) {
                    const DispCollLeaf& dispcoll_leaf = dispcoll_leaf_hits[dispcoll_leaf_hit_idx];

                    std::string label2 = "DispColl Leaf #" + std::to_string(dispcoll_leaf.dispcoll_leaf_idx);
                    if (ImGui::Selectable(label2.c_str(), false)) {

                    }
                }
            }
            ImGui::Unindent();
        }
        ImGui::Unindent();
    }

}
