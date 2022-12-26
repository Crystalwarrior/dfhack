#include "modules/ImTuiImpl.h"
#include "modules/Screen.h"
#include "ColorText.h"
#include "df/enabler.h"
#include "df/interface_key.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "MiscUtils.h"

using namespace DFHack;

#include <cmath>
#include <vector>
#include <algorithm>
#include <cctype>

int ImTuiInterop::name_to_colour_index(const std::string& name)
{
    std::map<std::string, int> names =
    {
        {"RESET",       -1},
        {"BLACK",        0},
        {"BLUE",         1},
        {"GREEN",        2},
        {"CYAN",         3},
        {"RED",          4},
        {"MAGENTA",      5},
        {"BROWN",        6},
        {"GREY",         7},
        {"DARKGREY",     8},
        {"LIGHTBLUE",    9},
        {"LIGHTGREEN",   10},
        {"LIGHTCYAN",    11},
        {"LIGHTRED",     12},
        {"LIGHTMAGENTA", 13},
        {"YELLOW",       14},
        {"WHITE",        15},
        {"MAX",          16},
    };

    return names.at(name);
}

//This isn't the only way that colour interop could be done
ImVec4 ImTuiInterop::colour_interop(std::vector<int> col3)
{
    col3.resize(3);

    //ImTui stuffs the character value in col.a
    return { static_cast<float>(col3[0]), static_cast<float>(col3[1]), static_cast<float>(col3[2]), 1.f };
}

//This isn't the only way that colour interop could be done
ImVec4 ImTuiInterop::colour_interop(std::vector<double> col3)
{
    col3.resize(3);

    //ImTui stuffs the character value in col.a
    return { static_cast<float>(col3[0]), static_cast<float>(col3[1]), static_cast<float>(col3[2]), 1.f };
}

ImVec4 ImTuiInterop::named_colours(const std::string& fg, const std::string& bg, bool bold)
{
    std::vector<int> vals;
    vals.resize(3);

    vals[0] = name_to_colour_index(fg);
    vals[1] = name_to_colour_index(bg);
    vals[2] = bold;

    return colour_interop(vals);
}

#define ABS(x) ((x >= 0) ? x : -x)

void ScanLine(int x1, int y1, int x2, int y2, int ymax, std::vector<int>& xrange) {
    int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

    sx = x2 - x1;
    sy = y2 - y1;

    if (sx > 0) dx1 = 1;
    else if (sx < 0) dx1 = -1;
    else dx1 = 0;

    if (sy > 0) dy1 = 1;
    else if (sy < 0) dy1 = -1;
    else dy1 = 0;

    m = ABS(sx);
    n = ABS(sy);
    dx2 = dx1;
    dy2 = 0;

    if (m < n)
    {
        m = ABS(sy);
        n = ABS(sx);
        dx2 = 0;
        dy2 = dy1;
    }

    x = x1; y = y1;
    cnt = m + 1;
    k = n / 2;

    while (cnt--) {
        if ((y >= 0) && (y < ymax)) {
            if (x < xrange[2 * y + 0]) xrange[2 * y + 0] = x;
            if (x > xrange[2 * y + 1]) xrange[2 * y + 1] = x;
        }

        k += n;
        if (k < m) {
            x += dx2;
            y += dy2;
        }
        else {
            k -= m;
            x += dx1;
            y += dy1;
        }
    }
}

void drawTriangle(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImU32 col) {
    df::coord2d dim = Screen::getWindowSize();

    std::vector<int> g_xrange;

    int screen_size = dim.x * dim.y;

    int ymin = std::floor(std::min(std::min(p0.y, p1.y), p2.y));
    int ymax = std::floor(std::max(std::max(p0.y, p1.y), p2.y));

    int ydelta = ymax - ymin + 1;

    if ((int) g_xrange.size() < 2*ydelta) {
        g_xrange.resize(2*ydelta);
    }

    for (int y = 0; y < ydelta; y++) {
        g_xrange[2*y+0] = 999999;
        g_xrange[2*y+1] = -999999;
    }

    p0.x = std::floor(p0.x);
    p1.x = std::floor(p1.x);
    p2.x = std::floor(p2.x);

    p0.y = std::floor(p0.y);
    p1.y = std::floor(p1.y);
    p2.y = std::floor(p2.y);

    ScanLine(p0.x, p0.y - ymin, p1.x, p1.y - ymin, ydelta, g_xrange);
    ScanLine(p1.x, p1.y - ymin, p2.x, p2.y - ymin, ydelta, g_xrange);
    ScanLine(p2.x, p2.y - ymin, p0.x, p0.y - ymin, ydelta, g_xrange);

    for (int y = 0; y < ydelta; y++) {
        if (g_xrange[2*y+1] >= g_xrange[2*y+0]) {
            int x = g_xrange[2*y+0];
            int len = 1 + g_xrange[2*y+1] - g_xrange[2*y+0];

            while (len--) {
                if (x >= 0 && x < dim.x && y + ymin >= 0 && y + ymin < dim.y) {
                    ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(col);

                    //todo: colours
                    const Screen::Pen pen(0, col4.x, col4.y);

                    Screen::paintString(pen, x, y + ymin, " ");
                }
                ++x;
            }
        }
    }
}

ImTuiInterop::ui_state& ImTuiInterop::get_global_ui_state()
{
    static ImTuiInterop::ui_state st = make_ui_system();

    return st;
}

void ImTuiInterop::impl::init_current_context()
{
    ImGui::GetStyle().Alpha = 1.0f;
    ImGui::GetStyle().WindowPadding = ImVec2(1.f, 0.0f);
    ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::GetStyle().WindowBorderSize = 0.0f;
    ImGui::GetStyle().WindowMinSize = ImVec2(4.0f, 1.0f);
    ImGui::GetStyle().WindowTitleAlign = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_Left;
    ImGui::GetStyle().ChildRounding = 0.0f;
    ImGui::GetStyle().ChildBorderSize = 0.0f;
    ImGui::GetStyle().PopupRounding = 0.0f;
    ImGui::GetStyle().PopupBorderSize = 0.0f;
    ImGui::GetStyle().FramePadding = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().FrameRounding = 0.0f;
    ImGui::GetStyle().FrameBorderSize = 0.0f;
    ImGui::GetStyle().ItemSpacing = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().ItemInnerSpacing = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().TouchExtraPadding = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().IndentSpacing = 1.0f;
    ImGui::GetStyle().ColumnsMinSpacing = 1.0f;
    ImGui::GetStyle().ScrollbarSize = 0.5f;
    ImGui::GetStyle().ScrollbarRounding = 0.0f;
    ImGui::GetStyle().GrabMinSize = 0.1f;
    ImGui::GetStyle().GrabRounding = 0.0f;
    ImGui::GetStyle().TabRounding = 0.0f;
    ImGui::GetStyle().TabBorderSize = 0.0f;
    ImGui::GetStyle().ColorButtonPosition = ImGuiDir_Right;
    //ImGui::GetStyle().ButtonTextAlign = ImVec2(0.5f, 0.0f);
    ImGui::GetStyle().ButtonTextAlign = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().SelectableTextAlign = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().DisplayWindowPadding = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().CellPadding = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().MouseCursorScale = 1.0f;
    ImGui::GetStyle().AntiAliasedLines = false;
    ImGui::GetStyle().AntiAliasedFill = false;
    ImGui::GetStyle().CurveTessellationTol = 1.25f;

    ImGuiStyle& style = ImGui::GetStyle();

    for (int i = 0; i < ImGuiCol_COUNT; i++)
    {
        style.Colors[i] = named_colours("BLACK", "BLACK", false);
    }

    //I really need a transparency colour
    style.Colors[ImGuiCol_Text] = named_colours("WHITE", "WHITE", false);
    style.Colors[ImGuiCol_TextDisabled] = named_colours("GREY", "GREY", false);
    style.Colors[ImGuiCol_TitleBg] = named_colours("BLACK", "BLUE", false);
    style.Colors[ImGuiCol_TitleBgActive] = named_colours("BLACK", "LIGHTBLUE", false);
    style.Colors[ImGuiCol_TitleBgCollapsed] = named_colours("BLACK", "BLUE", false);
    style.Colors[ImGuiCol_MenuBarBg] = named_colours("BLACK", "BLUE", false);

    style.Colors[ImGuiCol_TextSelectedBg] = named_colours("BLACK", "RED", false);

    //unsure about much of this
    style.Colors[ImGuiCol_CheckMark] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_SliderGrab] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_SliderGrabActive] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_Button] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_ButtonHovered] = named_colours("BLACK", "RED", false); //?
    style.Colors[ImGuiCol_ButtonActive] = named_colours("BLACK", "GREEN", false); //?
    style.Colors[ImGuiCol_Header] = named_colours("BLACK", "BLUE", false); //?
    style.Colors[ImGuiCol_HeaderHovered] = named_colours("BLACK", "BLUE", false); //?
    style.Colors[ImGuiCol_HeaderActive] = named_colours("BLACK", "BLUE", false); //?
    style.Colors[ImGuiCol_Separator] = named_colours("WHITE", "WHITE", false); //?
    style.Colors[ImGuiCol_SeparatorHovered] = named_colours("WHITE", "WHITE", false); //?
    style.Colors[ImGuiCol_SeparatorActive] = named_colours("WHITE", "WHITE", false); //?
    style.Colors[ImGuiCol_ResizeGrip] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_ResizeGripHovered] = named_colours("WHITE", "BLACK", false); //?
    style.Colors[ImGuiCol_ResizeGripActive] = named_colours("WHITE", "BLACK", false); //?

    style.Colors[ImGuiCol_TableBorderStrong] = named_colours("WHITE", "WHITE", false);
    style.Colors[ImGuiCol_TableBorderLight] = named_colours("WHITE", "WHITE", false);

    //good for debugging
    //style.Colors[ImGuiCol_NavHighlight] = named_colours("WHITE", "YELLOW", false);
    //style.Colors[ImGuiCol_NavWindowingHighlight] = named_colours("WHITE", "YELLOW", false);
    //style.Colors[ImGuiCol_NavWindowingDimBg] = named_colours("WHITE", "YELLOW", false);

    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0,0,0,0);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0);

    ImFontConfig fontConfig;
    fontConfig.GlyphMinAdvanceX = 1.0f;
    fontConfig.SizePixels = 1.00;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    // Build atlas
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

    ImGui::GetIO().KeyMap[ImGuiKey_Backspace] = df::enums::interface_key::STRING_A000; //why
    ImGui::GetIO().KeyMap[ImGuiKey_Escape] = df::enums::interface_key::LEAVESCREEN;
    //ImGui::GetIO().KeyMap[ImGuiKey_Enter] = df::enums::interface_key::SELECT;
    //ImGui::GetIO().KeyMap[ImGuiKey_Space] = df::enums::interface_key::STRING_A032;
    //So. ImGui uses space to focus/toggle buttons and widgets
    //But it uses Enter to activate eg an inputtext box
    //Widgets that use enter to activate explicitly (eg inputtext)
    //Have been modified to auto activate, and so I can bind 'space' aka activate
    //To the enter key to match dwarf fortress expectations
    ImGui::GetIO().KeyMap[ImGuiKey_Space] = df::enums::interface_key::SELECT;

    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow] = df::enums::interface_key::CURSOR_LEFT;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow] = df::enums::interface_key::CURSOR_RIGHT;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow] = df::enums::interface_key::CURSOR_UP;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow] = df::enums::interface_key::CURSOR_DOWN;

    //list of unmapped keys and their functions in ImGui:
    //if I could get ctrl + tab, would be able to cycle through windows with the keyboard
    //I beleive tab is also used for cycling through elements in keyboard navigation
    //ctrl + arrows would also allow word skipping
    //pageup/down would be ideal for large windows
    //ctrl c and v would allow for copypaste support, although its theoretically doable on an arbitrary keybind
    //and ctrl-x for cut
    //inputtext supports undo and redo on ctrl-z and ctrl-y
    //ctrl-a would be nice for select all
    //home and end I believe also navigate text
    //delete would be good for text deletion
    //unsure on the functionality of insert

    ImGui::GetIO().MouseDragThreshold = 0.f;

    df::coord2d dim = Screen::getWindowSize();
    ImGui::GetIO().DisplaySize = ImVec2(dim.x, dim.y);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

//so, the way that the existing widgets work is that if you hit eg 4, the printable
//character (ie '4') is processed first, and CURSOR_LEFT is implicitly ignored
//this takes the approach of explicitly killing the inputs from the input stream
std::set<df::interface_key> cleanup_keys(std::set<df::interface_key> keys, std::map<df::interface_key, int>& danger_key_time)
{
    std::map<df::interface_key, std::vector<df::interface_key>> to_kill_if_seen;

    to_kill_if_seen[Screen::charToKey('4')] = { { df::enums::interface_key::CURSOR_LEFT } };
    to_kill_if_seen[Screen::charToKey('6')] = { { df::enums::interface_key::CURSOR_RIGHT } };
    to_kill_if_seen[Screen::charToKey('8')] = { { df::enums::interface_key::CURSOR_UP } };
    to_kill_if_seen[Screen::charToKey('2')] = { { df::enums::interface_key::CURSOR_DOWN } };

    for (auto it : to_kill_if_seen)
    {
        auto found_trigger = keys.find(it.first);

        if (found_trigger == keys.end())
            continue;

        danger_key_time[it.first] = 0;

        for (df::interface_key c : it.second)
        {
            keys.erase(c);
        }
    }

    //I hate this
    //If you hold down 4 the game also sends a cursor left
    //when input is repeated, the game will send a second cursor left
    //but the second cursor left might arrive on the frame *before* the frame
    //where we get a 4 sent
    //which means that the cursor jumps left
    //current overlay.lua testing shows that current dfhack input does suffer from this issue
    //alternate solution would be to buffer arrow keys for a few frames
    //but I don't like the input latency in a game built around mashing arrow keys. This solution creates a long delay
    //when hitting eg 4 and then left, 8 + up, 6 + right, or 2 + down, but its such a rare case *anyway*
    //it doesn't seem like the end of the world
    //10 is totally arbitrary based on testing on my pc, should likely be time based
    int max_suppress_frames = 10;

    for (auto it : danger_key_time)
    {
        if (it.second <= max_suppress_frames)
        {
            std::vector<df::interface_key> to_kill = to_kill_if_seen[it.first];

            for (df::interface_key c : to_kill)
            {
                keys.erase(c);
            }
        }
    }

    return keys;
}

void ImTuiInterop::impl::new_frame(std::set<df::interface_key> keys, ui_state& st)
{
    keys = cleanup_keys(keys, st.danger_key_frames);

    for (auto& it : st.danger_key_frames)
    {
        it.second++;
    }

    ImGuiIO& io = ImGui::GetIO();

    reset_input();

    auto& keysDown = io.KeysDown;

    for (const df::interface_key& key : keys)
    {
        keysDown[key] = true;

        int charval = Screen::keyToChar(key);

        if(charval >= 0 && charval < 256 && std::isprint(charval))
          io.AddInputCharacter(charval);
    }

    df::coord2d dim = Screen::getWindowSize();
    ImGui::GetIO().DisplaySize = ImVec2(dim.x, dim.y);

    df::coord2d mouse_pos = Screen::getMousePos();

    io.MousePos.x = mouse_pos.x;
    io.MousePos.y = mouse_pos.y;

    //todo: frametime
    io.DeltaTime = 33.f / 1000.f;

    io.MouseDown[0] = st.pressed_mouse_keys[0];
    io.MouseDown[1] = st.pressed_mouse_keys[1];

    st.pressed_mouse_keys = {};

    ImGui::NewFrame();
}

void ImTuiInterop::impl::draw_frame(ImDrawData* drawData)
{
    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            {
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    float lastCharX = -10000.0f;
                    float lastCharY = -10000.0f;

                    for (unsigned int i = 0; i < pcmd->ElemCount; i += 3) {
                        int vidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 0];
                        int vidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 1];
                        int vidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 2];

                        auto pos0 = cmd_list->VtxBuffer[vidx0].pos;
                        auto pos1 = cmd_list->VtxBuffer[vidx1].pos;
                        auto pos2 = cmd_list->VtxBuffer[vidx2].pos;

                        auto uv0 = cmd_list->VtxBuffer[vidx0].uv;
                        auto uv1 = cmd_list->VtxBuffer[vidx1].uv;
                        auto uv2 = cmd_list->VtxBuffer[vidx2].uv;

                        auto col0 = cmd_list->VtxBuffer[vidx0].col;
                        //auto col1 = cmd_list->VtxBuffer[vidx1].col;
                        //auto col2 = cmd_list->VtxBuffer[vidx2].col;

                        if (uv0.x != uv1.x || uv0.x != uv2.x || uv1.x != uv2.x ||
                            uv0.y != uv1.y || uv0.y != uv2.y || uv1.y != uv2.y) {
                            int vvidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 3];
                            int vvidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 4];
                            int vvidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 5];

                            auto ppos0 = cmd_list->VtxBuffer[vvidx0].pos;
                            auto ppos1 = cmd_list->VtxBuffer[vvidx1].pos;
                            auto ppos2 = cmd_list->VtxBuffer[vvidx2].pos;

                            float x = ((pos0.x + pos1.x + pos2.x + ppos0.x + ppos1.x + ppos2.x) / 6.0f);
                            float y = ((pos0.y + pos1.y + pos2.y + ppos0.y + ppos1.y + ppos2.y) / 6.0f) + 0.5f;

                            if (std::fabs(y - lastCharY) < 0.5f && std::fabs(x - lastCharX) < 0.5f) {
                                x = lastCharX + 1.0f;
                                y = lastCharY;
                            }

                            lastCharX = x;
                            lastCharY = y;

                            int xx = static_cast<int>(std::floor(x));
                            int yy = static_cast<int>(std::floor(y));
                            if (xx < clip_rect.x || xx >= clip_rect.z || yy < clip_rect.y || yy >= clip_rect.w) {
                            }
                            else {
                                int slen = strlen(&cmd_list->VtxBuffer[vidx0].chrs[0]);

                                std::string as_utf8(&cmd_list->VtxBuffer[vidx0].chrs[0], slen);

                                ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(col0);

                                const Screen::Pen current_bg = Screen::readTile(xx, yy);

                                std::string as_df = UTF2DF(as_utf8);

                                if (as_df.size() == 1)
                                {
                                    //I am text, and have no background
                                    const Screen::Pen pen(as_df[0], col4.x, current_bg.bg);

                                    //Screen::paintString(pen, xx, yy, std::string(1, c));
                                    Screen::paintTile(pen, xx, yy);
                                }
                                else
                                {
                                    const Screen::Pen pen('?', col4.x, current_bg.bg);

                                    Screen::paintTile(pen, xx, yy);
                                }
                            }
                            i += 3;
                        }
                        else {
                            //todo: pass in clip rect
                            drawTriangle(pos0, pos1, pos2, col0);
                        }
                    }
                }
            }
        }
    }
}

void ImTuiInterop::impl::shutdown()
{

}

void ImTuiInterop::impl::reset_input()
{
    ImGuiIO& io = ImGui::GetIO();

    int arraysize_of_keysdown = IM_ARRAYSIZE(io.KeysDown);
    int max_df_keys = df::enum_traits<df::interface_key>::last_item_value + 1;

    //use an extra slot as an always-false key
    max_df_keys += 1;

    assert(arraysize_of_keysdown >= max_df_keys);

    auto& keysDown = io.KeysDown;
    std::fill(keysDown, keysDown + arraysize_of_keysdown, false);

    std::fill(io.MouseDown, io.MouseDown + 5, false);
}

ImTuiInterop::ui_state::ui_state()
{
    last_context = nullptr;
    ctx = nullptr;
}

ImTuiInterop::ui_state::~ui_state()
{

}

void ImTuiInterop::ui_state::feed(std::set<df::interface_key> keys)
{
    unprocessed_keys.insert(keys.begin(), keys.end());

    pressed_mouse_keys = {};

    if (df::global::enabler) {
        if (df::global::enabler->mouse_lbut || df::global::enabler->mouse_lbut_down) {
            pressed_mouse_keys[0] = 1;
        }
        if (df::global::enabler->mouse_rbut || df::global::enabler->mouse_rbut_down) {
            pressed_mouse_keys[1] = 1;
        }
    }
}

void ImTuiInterop::ui_state::activate()
{
    last_context = ImGui::GetCurrentContext();

    ImGui::SetCurrentContext(ctx);
}

void ImTuiInterop::ui_state::new_frame()
{
    ImTuiInterop::impl::new_frame(std::move(unprocessed_keys), *this);
    unprocessed_keys.clear();
}

void ImTuiInterop::ui_state::draw_frame(ImDrawData* drawData)
{
    ImTuiInterop::impl::draw_frame(drawData);
}

void ImTuiInterop::ui_state::deactivate()
{
    ImGui::SetCurrentContext(last_context);
}

void ImTuiInterop::ui_state::reset_input()
{
    ImTuiInterop::impl::reset_input();
}

ImTuiInterop::ui_state ImTuiInterop::make_ui_system()
{
    ImGuiContext* ctx = ImGui::CreateContext();

    ui_state st;
    st.ctx = ctx;

    st.activate();

    impl::init_current_context();

    st.deactivate();

    return st;
}

void ImTuiInterop::viewscreen::claim_current_imgui_window()
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();

    ImTuiInterop::ui_state& st = ImTuiInterop::get_global_ui_state();

    st.windows[st.render_stack].push_back(win->Name);
}

void ImTuiInterop::viewscreen::suppress_next_keyboard_feed_upwards()
{
    ImTuiInterop::get_global_ui_state().suppress_next_keyboard_passthrough = true;
}

void ImTuiInterop::viewscreen::suppress_next_mouse_feed_upwards()
{
    ImTuiInterop::get_global_ui_state().suppress_next_keyboard_passthrough = true;
}

void ImTuiInterop::viewscreen::feed_upwards()
{
    ImTuiInterop::get_global_ui_state().should_pass_keyboard_up = true;
}

void ImTuiInterop::viewscreen::declare_suppressed_key(df::interface_key key)
{
    ImTuiInterop::ui_state& st = ImTuiInterop::get_global_ui_state();
    st.suppressed_keys[st.render_stack].insert(key);
}

int ImTuiInterop::viewscreen::on_render_start(bool is_top)
{
    ImTuiInterop::ui_state& st = ImTuiInterop::get_global_ui_state();

    if (is_top)
    {
        st.windows.clear();
        st.rendered_windows.clear();
        st.render_stack = 0;
        st.suppressed_keys.clear();
        st.activate();
        st.new_frame();
    }

    st.render_stack++;

    int my_render_stack = st.render_stack;

    return my_render_stack;
}

//this function finds windows by name, and handles top
static void imgui_build_windows(std::vector<ImGuiWindow*>& out, const std::vector<std::string>& name, std::set<std::string>& ignore, bool is_top)
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();

    std::set<std::string> set_names(name.begin(), name.end());

    for (int i = 0; i < ctx->Windows.Size; i++)
    {
        ImGuiWindow* next = ctx->Windows[i];

        std::string sname(next->Name);

        if (set_names.count(sname) == 0 && !is_top)
            continue;

        if (ignore.count(sname))
            continue;

        out.push_back(next);
    }
}

static void imgui_append(std::vector<ImGuiWindow*>& out, std::set<std::string>& should_ignore, ImGuiWindow* win)
{
    assert(win);

    if(should_ignore.count(win->Name) == 0)
        out.push_back(win);

    for (int i = 0; i < win->DC.ChildWindows.Size; i++)
    {
        imgui_append(out, should_ignore, win->DC.ChildWindows[i]);
    }
}

static void imgui_append_children(std::vector<ImGuiWindow*> in, std::set<std::string>& should_ignore, std::vector<ImGuiWindow*>& out)
{
    for (auto i : in)
    {
        imgui_append(out, should_ignore, i);
    }
}

static std::vector<ImGuiWindow*> imgui_pull_from_context_in_display_order(std::vector<ImGuiWindow*> windows)
{
    std::set<ImGuiWindow*> set_windows(windows.begin(), windows.end());

    ImGuiContext* ctx = ImGui::GetCurrentContext();

    std::vector<ImGuiWindow*> result;

    for (int i = 0; i < ctx->Windows.Size; i++)
    {
        ImGuiWindow* win = ctx->Windows[i];

        if (set_windows.count(win) == 0)
            continue;

        result.push_back(win);
    }

    return result;
}

static std::vector<ImGuiWindow*> imgui_pull_from_context_in_focus_order(std::vector<ImGuiWindow*> windows)
{
    std::set<ImGuiWindow*> set_windows(windows.begin(), windows.end());

    ImGuiContext* ctx = ImGui::GetCurrentContext();

    std::vector<ImGuiWindow*> result;

    for (int i = 0; i < ctx->WindowsFocusOrder.Size; i++)
    {
        ImGuiWindow* win = ctx->WindowsFocusOrder[i];

        if (set_windows.count(win) == 0)
            continue;

        result.push_back(win);
    }

    return result;
}

//only do this to display order
static std::vector<ImGuiWindow*> imgui_child_sort(std::vector<ImGuiWindow*> in)
{
    ImGui::SortWindows(in);
    return in;
}

static void imgui_rearrange_internals(const std::vector<ImGuiWindow*>& display_order, const std::vector<ImGuiWindow*>& focus_order)
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();

    std::set<ImGuiWindow*> set_display_order(display_order.begin(), display_order.end());
    std::set<ImGuiWindow*> set_focus_order(focus_order.begin(), focus_order.end());

    ImVector<ImGuiWindow*> finished_display_order;
    ImVector<ImGuiWindow*> finished_focus_order;

    //ImGui represents display order back to front, ie Windows.back() is considered the front
    //so here we first push all the existing windows *except* for the ones we're actually going to render
    //and then push the windows we're interested into the front
    //importantly, they're all in the original order they were in in the list
    for (auto win : ctx->Windows)
    {
        if (set_display_order.count(win) > 0)
            continue;

        finished_display_order.push_back(win);
    }

    for (auto win : display_order)
    {
        finished_display_order.push_back(win);
    }

    for (auto win : ctx->WindowsFocusOrder)
    {
        if (set_focus_order.count(win) > 0)
            continue;

        finished_focus_order.push_back(win);
    }

    for (auto win : focus_order)
    {
        finished_focus_order.push_back(win);
    }

    ctx->Windows = finished_display_order;
    ctx->WindowsFocusOrder = finished_focus_order;
}

void ImTuiInterop::viewscreen::on_render_end(bool is_top, int id)
{
    ImTuiInterop::ui_state& st = ImTuiInterop::get_global_ui_state();

    bool respect_dwarf_fortress_viewscreen_order = true;

    if (respect_dwarf_fortress_viewscreen_order)
    {
        std::vector<std::string> my_windows = st.windows[id];

        std::vector<ImGuiWindow*> unsorted_windows_without_children;
        imgui_build_windows(unsorted_windows_without_children, my_windows, st.rendered_windows, is_top);

        std::vector<ImGuiWindow*> unsorted_windows;
        imgui_append_children(unsorted_windows_without_children, st.rendered_windows, unsorted_windows);

        for (ImGuiWindow* win : unsorted_windows)
        {
            st.rendered_windows.insert(win->Name);
        }

        std::vector<ImGuiWindow*> display_order = imgui_pull_from_context_in_display_order(unsorted_windows);
        std::vector<ImGuiWindow*> focus_order = imgui_pull_from_context_in_focus_order(unsorted_windows);

        display_order = imgui_child_sort(display_order);

        imgui_rearrange_internals(display_order, focus_order);

        std::set<std::string> none;

        //render only the windows which I own, which are named by st.rendered_windows
        //any unnamed windows will be rendered when is_top is true
        //todo: implicitly discover windows, this is slightly brittle
        //as only explicitly named windows via imgui_begin are noticed
        ImGui::ProgressiveRender(display_order, none, is_top);

        //does not look at render stack
        st.draw_frame(ImGui::GetDrawData());
    }

    st.render_stack--;

    if (is_top)
    {
        ImGui::EndFrame();

        if (!respect_dwarf_fortress_viewscreen_order)
        {
            ImGui::Render();
            st.draw_frame(ImGui::GetDrawData());
        }

        st.deactivate();
    }
}

void ImTuiInterop::viewscreen::on_feed_start(bool is_top, std::set<df::interface_key>* keys)
{
    ImTuiInterop::ui_state& st = ImTuiInterop::get_global_ui_state();

    if (keys && is_top)
    {
        st.feed(*keys);
    }

    st.activate();
}

bool ImTuiInterop::viewscreen::on_feed_end(std::set<df::interface_key>* keys)
{
    ImTuiInterop::ui_state& st = ImTuiInterop::get_global_ui_state();

    bool should_feed = false;

    //So, while this passes keyboard inputs up
    //The current code structure seems to intentionally suppresses mouse clicks from filtering up
    //through multiple lua scripts, by setting the lmouse_down in the global
    //enabler to 0 in pushinterfacekeys
    //this seems undesirable for imgui windows to unconditionally forcibly
    //suppress mouse clicks
    if (st.should_pass_keyboard_up && !st.suppress_next_keyboard_passthrough && keys)
    {
        bool skip_feed = false;

        for (auto it : st.suppressed_keys)
        {
            for (auto key : it.second)
            {
                if (keys->count(df::interface_key(key)) > 0)
                {
                    skip_feed = true;
                    break;
                }
            }

            if (skip_feed)
                break;
        }

        should_feed = !skip_feed;
    }

    st.suppress_next_keyboard_passthrough = false;
    st.should_pass_keyboard_up = false;

    st.deactivate();

    return should_feed;
}

void ImTuiInterop::viewscreen::on_dismiss_final_imgui_aware_viewscreen()
{
    ImTuiInterop::get_global_ui_state().reset_input();
    ImTuiInterop::get_global_ui_state().suppressed_keys.clear();
}
