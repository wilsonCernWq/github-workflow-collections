// ======================================================================== //
// Copyright Qi Wu, since 2019                                              //
// Copyright SCI Institute, University of Utah, 2018                        //
// ======================================================================== //
#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "core.h"
#include "default.h"

#include <imconfig.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tfn {

class TFN_MODULE_INTERFACE TransferFunctionWidget
{
 private:
  using setter = std::function<void(const list1f &, const list1f &, const vec2f &)>;

 private:
  /* Variables Controlled by Users */
  setter _setter_cb;
  vec2f valueRange; //< the current value range controlled by the user
  vec2f defaultRange; //< the default value range being displayed on the GUI
  /*
   * Flags
   *   tfn_changed: Track if the function changed and must be re-uploaded. We start by marking it changed to upload the initial palette
   */
  bool tfn_changed{true};
  /*
   * OpenGL Variables
   *   tfn_palette: The 2d palette texture on the GPU for displaying the color map preview in the UI.
   */
  GLuint tfn_palette;

  // TODO: below to be cleaned

  // This MAYBE the correct way of doing this
  /*       struct TFN { */
  /* 	std::vector<ColorPoint>   colors; */
  /* 	std::vector<OpacityPoint_Linear> opacity; */
  /* 	tfn::TransferFunction reader; */
  /*       }; */
  std::vector<tfn::TransferFunction> tfn_readers;

  // TFNs information:
  std::vector<bool> tfn_editable;
  std::vector<std::vector<ColorPoint>> tfn_c_list;
  std::vector<std::vector<OpacityPoint_Linear>> tfn_o_list;

  std::vector<ColorPoint> *tfn_c;
  std::vector<OpacityPoint_Linear> *tfn_o;
  std::vector<std::string> tfn_names; // name of TFNs in the dropdown menu

  // State Variables:
  // TODO: those variables might be unnecessary
  bool tfn_edit;
  std::vector<char> tfn_text_buffer; // The filename input text buffer

  // Meta Data
  // * Interpolated trasnfer function size
  // int tfn_w = 256;
  // int tfn_h = 1; // TODO: right now we only support 1D TFN

  // * The selected transfer function being shown
  int tfn_selection;

 public:
  ~TransferFunctionWidget();
  TransferFunctionWidget(const setter &);
  TransferFunctionWidget(const TransferFunctionWidget &);
  TransferFunctionWidget &operator=(const TransferFunctionWidget &);
  /* Setup the default data value range for the transfer function */
  void set_default_value_range(const float &a, const float &b);
  /* Draw the transfer function editor widget, returns true if the transfer function changed */
  bool build(bool *p_open = NULL, bool do_not_render_textures = false);
  /* Construct the ImGui GUI */
  void build_gui();
  /* Render the transfer function to a 1D texture that can be applied to volume data */
  void render(int tfn_w = 256, int tfn_h = 1);
  /** Load the transfer function in the file passed and set it active */
  void load(const std::string &fileName);
  /** Save the current transfer function out to the file */
  void save(const std::string &fileName) const;

  void add_tfn(const list1f &, const list1f &, const std::string &name);

 private:
  /** Namely, make a selected transfer function table current */
  void set_tfn(int);
  /** Load all the pre-defined transfer function maps */
  void set_default_tfns();
  /** Draw the Tfn Editor in a window */
  void draw_tfn_editor(const float margin, const float height);
  tfn::vec4f draw_tfn_editor__preview_texture(void *_draw_list, const tfn::vec3f &, const tfn::vec2f &, const tfn::vec4f &);
  tfn::vec4f draw_tfn_editor__color_control_points(void *_draw_list, const tfn::vec3f &, const tfn::vec2f &, const tfn::vec4f &, const float &);
  tfn::vec4f draw_tfn_editor__opacity_control_points(void *_draw_list, const tfn::vec3f &, const tfn::vec2f &, const tfn::vec4f &, const float &);
  tfn::vec4f draw_tfn_editor__interaction_blocks(void *_draw_list, const tfn::vec3f &, const tfn::vec2f &, const tfn::vec4f &, const float &, const float &);
};

inline TransferFunctionWidget::~TransferFunctionWidget()
{
  if (tfn_palette)
    glDeleteTextures(1, &tfn_palette);
}

inline TransferFunctionWidget::TransferFunctionWidget(const setter &fcn)
    : tfn_selection(0), tfn_changed(true), tfn_palette(0), tfn_text_buffer(512, '\0'), _setter_cb(fcn), valueRange{0.f, 0.f}, defaultRange{0.f, 0.f}
{
  set_default_tfns();
  tfn_c = &(tfn_c_list[tfn_selection]);
  tfn_o = &(tfn_o_list[tfn_selection]);
  tfn_edit = tfn_editable[tfn_selection];
}

inline TransferFunctionWidget::TransferFunctionWidget(const TransferFunctionWidget &core)
    : tfn_c_list(core.tfn_c_list),
      tfn_o_list(core.tfn_o_list),
      tfn_readers(core.tfn_readers),
      tfn_selection(core.tfn_selection),
      tfn_changed(true),
      tfn_palette(0),
      tfn_text_buffer(512, '\0'),
      _setter_cb(core._setter_cb),
      valueRange(core.valueRange),
      defaultRange(core.defaultRange)
{
  tfn_c = &(tfn_c_list[tfn_selection]);
  tfn_o = &(tfn_o_list[tfn_selection]);
  tfn_edit = tfn_editable[tfn_selection];
}

inline TransferFunctionWidget &TransferFunctionWidget::operator=(const TransferFunctionWidget &core)
{
  if (this == &core) {
    return *this;
  }
  tfn_c_list = core.tfn_c_list;
  tfn_o_list = core.tfn_o_list;
  tfn_readers = core.tfn_readers;
  tfn_selection = core.tfn_selection;
  tfn_changed = true;
  tfn_palette = 0;
  this->_setter_cb = core._setter_cb;
  valueRange = core.valueRange;
  defaultRange = core.defaultRange;
  return *this;
}

inline void TransferFunctionWidget::set_tfn(int selection)
{
  if (tfn_selection != selection) {
    tfn_selection = selection; // Remember to update other constructors also
    tfn_c = &(tfn_c_list[selection]);
    tfn_o = &(tfn_o_list[selection]);
    tfn_edit = tfn_editable[selection];
    tfn_changed = true;
  }
}

inline void TransferFunctionWidget::add_tfn(const list1f &ct, const list1f &ot, const std::string &name)
{
  auto it = std::find(tfn_names.begin(), tfn_names.end(), name);

  if (it == tfn_names.end()) {
    tfn_c_list.emplace_back(ct.size() / 4);
    for (size_t i = 0; i < ct.size() / 4; ++i) {
      tfn_c_list.back()[i] = ColorPoint(ct[i * 4], ct[i * 4 + 1], ct[i * 4 + 2], ct[i * 4 + 3]);
    }
    tfn_o_list.emplace_back(ot.size() / 2);
    for (size_t i = 0; i < ot.size() / 2; ++i) {
      tfn_o_list.back()[i] = OpacityPoint_Linear(ot[i * 2], ot[i * 2 + 1]);
    }
    tfn_editable.push_back(false);
    tfn_names.push_back(name);
    tfn_selection = (int)(tfn_c_list.size() - 1); // Remember to update other constructors also
  } else {
    tfn_selection = (int)std::distance(tfn_names.begin(), it);
  }
  tfn_c = &(tfn_c_list[tfn_selection]);
  tfn_o = &(tfn_o_list[tfn_selection]);
  tfn_edit = tfn_editable[tfn_selection];
  tfn_changed = true;
}

inline void TransferFunctionWidget::set_default_value_range(const float &a, const float &b)
{
  valueRange.x = defaultRange.x = a;
  valueRange.y = defaultRange.y = b;
  tfn_changed = true;
}

inline tfn::vec4f TransferFunctionWidget::draw_tfn_editor__preview_texture(void *_draw_list,
    const tfn::vec3f &margin, /* left, right, spacing*/
    const tfn::vec2f &size,
    const tfn::vec4f &cursor)
{
  auto draw_list = (ImDrawList *)_draw_list;
  ImGui::SetCursorScreenPos(ImVec2(cursor.x + margin.x, cursor.y));
  ImGui::Image(reinterpret_cast<void *>(tfn_palette), (const ImVec2 &)size);
  ImGui::SetCursorScreenPos((const ImVec2 &)cursor);
  // TODO: more generic way of drawing arbitary splats
  for (int i = 0; i < tfn_o->size() - 1; ++i) {
    std::vector<ImVec2> polyline;
    polyline.emplace_back(cursor.x + margin.x + (*tfn_o)[i].p * size.x, cursor.y + size.y);
    polyline.emplace_back(cursor.x + margin.x + (*tfn_o)[i].p * size.x, cursor.y + (1.f - (*tfn_o)[i].a) * size.y);
    polyline.emplace_back(cursor.x + margin.x + (*tfn_o)[i + 1].p * size.x + 1, cursor.y + (1.f - (*tfn_o)[i + 1].a) * size.y);
    polyline.emplace_back(cursor.x + margin.x + (*tfn_o)[i + 1].p * size.x + 1, cursor.y + size.y);
#ifdef IMGUI_VERSION_NUM
    draw_list->AddConvexPolyFilled(polyline.data(), (int)polyline.size(), 0xFFD8D8D8 /*, true*/);
#else
    draw_list->AddConvexPolyFilled(polyline.data(), (int)polyline.size(), 0xFFD8D8D8, true);
#endif
  }
  tfn::vec4f new_cursor = {
      cursor.x,
      cursor.y + size.y + margin.z,
      cursor.z,
      cursor.w - size.y,
  };
  ImGui::SetCursorScreenPos((const ImVec2 &)new_cursor);
  return new_cursor;
}

inline tfn::vec4f TransferFunctionWidget::draw_tfn_editor__color_control_points(void *_draw_list,
    const tfn::vec3f &margin, /* left, right, spacing*/
    const tfn::vec2f &size,
    const tfn::vec4f &cursor,
    const float &color_len)
{
  auto draw_list = (ImDrawList *)_draw_list;
  // draw circle background
  draw_list->AddRectFilled(
      ImVec2(cursor.x + margin.x, cursor.y - margin.z), ImVec2(cursor.x + margin.x + size.x, cursor.y - margin.x + 2.5 * color_len), 0xFF474646);
  // draw circles
  for (int i = tfn_c->size() - 1; i >= 0; --i) {
    const ImVec2 pos(cursor.x + size.x * (*tfn_c)[i].p + margin.x, cursor.y);
    ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y));
    // white background
    draw_list->AddTriangleFilled(
        ImVec2(pos.x - 0.5f * color_len, pos.y), ImVec2(pos.x + 0.5f * color_len, pos.y), ImVec2(pos.x, pos.y - color_len), 0xFFD8D8D8);
    draw_list->AddCircleFilled(ImVec2(pos.x, pos.y + 0.5f * color_len), color_len, 0xFFD8D8D8);
    // draw picker
    ImVec4 picked_color = ImColor((*tfn_c)[i].r, (*tfn_c)[i].g, (*tfn_c)[i].b, 1.f);
    ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y + 1.5f * color_len));
    if (ImGui::ColorEdit4(("##ColorPicker" + std::to_string(i)).c_str(),
            (float *)&picked_color,
            ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreview
                | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip)) {
      (*tfn_c)[i].r = picked_color.x;
      (*tfn_c)[i].g = picked_color.y;
      (*tfn_c)[i].b = picked_color.z;
      tfn_changed = true;
    }
    if (ImGui::IsItemHovered()) {
      // convert float color to char
      int cr = static_cast<int>(picked_color.x * 255);
      int cg = static_cast<int>(picked_color.y * 255);
      int cb = static_cast<int>(picked_color.z * 255);
      // setup tooltip
      ImGui::BeginTooltip();
      ImVec2 sz(ImGui::GetFontSize() * 4 + ImGui::GetStyle().FramePadding.y * 2, ImGui::GetFontSize() * 4 + ImGui::GetStyle().FramePadding.y * 2);
      ImGui::ColorButton("##PreviewColor", picked_color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview, sz);
      ImGui::SameLine();
      ImGui::Text(
          "Left click to edit\n"
          "HEX: #%02X%02X%02X\n"
          "RGB: [%3d,%3d,%3d]\n(%.2f, %.2f, %.2f)",
          cr,
          cg,
          cb,
          cr,
          cg,
          cb,
          picked_color.x,
          picked_color.y,
          picked_color.z);
      ImGui::EndTooltip();
    }
  }
  for (int i = 0; i < tfn_c->size(); ++i) {
    const ImVec2 pos(cursor.x + size.x * (*tfn_c)[i].p + margin.x, cursor.y);
    // draw button
    ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y - 0.5f * color_len));
    ImGui::InvisibleButton(("##ColorControl-" + std::to_string(i)).c_str(), ImVec2(2.f * color_len, 2.f * color_len));
    // dark highlight
    ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y));
    draw_list->AddCircleFilled(ImVec2(pos.x, pos.y + 0.5f * color_len), 0.5f * color_len, ImGui::IsItemHovered() ? 0xFF051C33 : 0xFFBCBCBC);
    // delete color point
    if (ImGui::IsMouseDoubleClicked(1) && ImGui::IsItemHovered()) {
      if (i > 0 && i < tfn_c->size() - 1) {
        tfn_c->erase(tfn_c->begin() + i);
        tfn_changed = true;
      }
    }
    // drag color control point
    else if (ImGui::IsItemActive()) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      if (i > 0 && i < tfn_c->size() - 1) {
        (*tfn_c)[i].p += delta.x / size.x;
        (*tfn_c)[i].p = clamp((*tfn_c)[i].p, (*tfn_c)[i - 1].p, (*tfn_c)[i + 1].p);
      }
      tfn_changed = true;
    }
  }
  return vec4f();
}

inline tfn::vec4f TransferFunctionWidget::draw_tfn_editor__opacity_control_points(/**/
    void *_draw_list,
    const tfn::vec3f &margin, /* left, right, spacing*/
    const tfn::vec2f &size,
    const tfn::vec4f &cursor,
    const float &opacity_len)
{
  auto draw_list = (ImDrawList *)_draw_list;
  // draw circles
  for (int i = 0; i < tfn_o->size(); ++i) {
    const ImVec2 pos(cursor.x + size.x * (*tfn_o)[i].p + margin.x, cursor.y - size.y * (*tfn_o)[i].a - margin.z);
    ImGui::SetCursorScreenPos(ImVec2(pos.x - opacity_len, pos.y - opacity_len));
    ImGui::InvisibleButton(("##OpacityControl-" + std::to_string(i)).c_str(), ImVec2(2.f * opacity_len, 2.f * opacity_len));
    ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y));
    // dark bounding box
    draw_list->AddCircleFilled(pos, opacity_len, 0xFF565656);
    // white background
    draw_list->AddCircleFilled(pos, 0.8f * opacity_len, 0xFFD8D8D8);
    // highlight
    draw_list->AddCircleFilled(pos, 0.6f * opacity_len, ImGui::IsItemHovered() ? 0xFF051c33 : 0xFFD8D8D8);
    // delete opacity point
    if (ImGui::IsMouseDoubleClicked(1) && ImGui::IsItemHovered()) {
      if (i > 0 && i < tfn_o->size() - 1) {
        tfn_o->erase(tfn_o->begin() + i);
        tfn_changed = true;
      }
    } else if (ImGui::IsItemActive()) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      (*tfn_o)[i].a -= delta.y / size.y;
      (*tfn_o)[i].a = clamp((*tfn_o)[i].a, 0.0f, 1.0f);
      if (i > 0 && i < tfn_o->size() - 1) {
        (*tfn_o)[i].p += delta.x / size.x;
        (*tfn_o)[i].p = clamp((*tfn_o)[i].p, (*tfn_o)[i - 1].p, (*tfn_o)[i + 1].p);
      }
      tfn_changed = true;
    }
  }
  return vec4f();
}

inline tfn::vec4f TransferFunctionWidget::draw_tfn_editor__interaction_blocks(/**/
    void *_draw_list,
    const tfn::vec3f &margin, /* left, right, spacing */
    const tfn::vec2f &size,
    const tfn::vec4f &cursor,
    const float &color_len,
    const float &opacity_len)
{
  const float mouse_x = ImGui::GetMousePos().x;
  const float mouse_y = ImGui::GetMousePos().y;
  const float scroll_x = ImGui::GetScrollX();
  const float scroll_y = ImGui::GetScrollY();
  auto draw_list = (ImDrawList *)_draw_list;
  ImGui::SetCursorScreenPos(ImVec2(cursor.x + margin.x, cursor.y - margin.z));
  ImGui::InvisibleButton("##tfn_palette_color", ImVec2(size.x, 2.5f * color_len));
  // add color point
  if (tfn_edit && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
    const float p = clamp((mouse_x - cursor.x - margin.x - scroll_x) / (float)size.x, 0.f, 1.f);
    int il, ir;
    std::tie(il, ir) = find_interval(tfn_c, p);
    const float pr = (*tfn_c)[ir].p;
    const float pl = (*tfn_c)[il].p;
    const float r = lerp((*tfn_c)[il].r, (*tfn_c)[ir].r, pl, pr, p);
    const float g = lerp((*tfn_c)[il].g, (*tfn_c)[ir].g, pl, pr, p);
    const float b = lerp((*tfn_c)[il].b, (*tfn_c)[ir].b, pl, pr, p);
    ColorPoint pt;
    pt.p = p, pt.r = r, pt.g = g, pt.b = b;
    tfn_c->insert(tfn_c->begin() + ir, pt);
    tfn_changed = true;
  }
  // draw background interaction
  ImGui::SetCursorScreenPos(ImVec2(cursor.x + margin.x, cursor.y - size.y - margin.z));
  ImGui::InvisibleButton("##tfn_palette_opacity", ImVec2(size.x, size.y));
  // add opacity point
  if (tfn_edit && ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
    const float x = clamp((mouse_x - cursor.x - margin.x - scroll_x) / (float)size.x, 0.f, 1.f);
    const float y = clamp(-(mouse_y - cursor.y + margin.x - scroll_y) / (float)size.y, 0.f, 1.f);
    int il, ir;
    std::tie(il, ir) = find_interval(tfn_o, x);
    OpacityPoint_Linear pt;
    pt.p = x, pt.a = y;
    tfn_o->insert(tfn_o->begin() + ir, pt);
    tfn_changed = true;
  }
  return vec4f();
}

inline void TransferFunctionWidget::draw_tfn_editor(const float margin, const float height)
{
  // style
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  const float canvas_x = ImGui::GetCursorScreenPos().x;
  float canvas_y = ImGui::GetCursorScreenPos().y;
  const float width = ImGui::GetContentRegionAvail().x - 2.f * margin;
  const float color_len = 10.f;
  const float opacity_len = 10.f;
  // debug
  const tfn::vec3f m{margin, margin, margin};
  const tfn::vec2f s{width, height};
  tfn::vec4f c = {canvas_x, canvas_y, ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
  // draw preview texture
  c = draw_tfn_editor__preview_texture(draw_list, m, s, c);
  canvas_y = c.y;
  // draw color control points
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
  if (tfn_edit) {
    draw_tfn_editor__color_control_points(draw_list, m, s, c, color_len);
  }
  // draw opacity control points
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
  if (tfn_edit) {
    draw_tfn_editor__opacity_control_points(draw_list, m, s, c, opacity_len);
  }
  // draw background interaction
  draw_tfn_editor__interaction_blocks(draw_list, m, s, c, color_len, opacity_len);
  // update cursors
  canvas_y += 4.f * color_len + margin;
  ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
}

inline bool TransferFunctionWidget::build(bool *p_open, bool do_not_render_textures)
{
  // ImGui::ShowTestWindow();

  ImGui::SetNextWindowSizeConstraints(ImVec2(400, 250), ImVec2(FLT_MAX, FLT_MAX));

  if (!ImGui::Begin("Transfer Function Widget", p_open)) {
    ImGui::End();
    return false;
  }

  build_gui();

  ImGui::End();

  if (!do_not_render_textures)
    render();

  return true;
}

void TransferFunctionWidget::build_gui()
{
  //------------ Styling ------------------------------
  const float margin = 10.f;

  //------------ Basic Controls -----------------------
  ImGui::Spacing();
  ImGui::SetCursorPosX(margin);
  ImGui::BeginGroup();
  {
    /* title */
    ImGui::Text("1D Transfer Function Editor");
    ImGui::SameLine();
    {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.f);
      ImGui::Button("help");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Double right click a control point to delete it\n"
            "Single left click and drag a control point to "
            "move it\n"
            "Double left click on an empty area to add a "
            "control point\n");
      }
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);
    }

    ImGui::Spacing();

    /* load a transfer function from file */
    ImGui::InputText("", tfn_text_buffer.data(), tfn_text_buffer.size() - 1);
    ImGui::SameLine();
    if (ImGui::Button("load a new tfn")) {
      try {
        std::string s = tfn_text_buffer.data();
        s.erase(s.find_last_not_of(" \n\r\t") + 1);
        s.erase(0, s.find_first_not_of(" \n\r\t"));
        load(s.c_str());
      } catch (const std::runtime_error &error) {
        std::cerr << "\033[1;33m"
                  << "Error: " << error.what() << "\033[0m" << std::endl;
      }
    }

    // TODO: save function is not implemented
    // if (ImGui::Button("save")) { save(tfn_text_buffer.data()); }

    /* Built-in color lists */
    {
      static int curr_tfn = tfn_selection;
      static std::string curr_names = "";
      curr_names = "";
      for (auto &n : tfn_names) {
        curr_names += n + '\0';
      }
      if (ImGui::Combo(" color tables", &curr_tfn, curr_names.c_str())) {
        set_tfn(curr_tfn);
      }
    }

    /* Display transfer function value range */
    static vec2f value_range_percentage(0.f, 100.f);
    if (defaultRange.y > defaultRange.x) {
      ImGui::Text(" default value range (%.6f, %.6f)", defaultRange.x, defaultRange.y);
      ImGui::Text(" current value range (%.6f, %.6f)", valueRange.x, valueRange.y);
      if (ImGui::DragFloat2(" value range %", (float *)&value_range_percentage, 1.f, 0.f, 100.f, "%.3f")) {
        tfn_changed = true;
        valueRange.x = (defaultRange.y - defaultRange.x) * value_range_percentage.x * 0.01f + defaultRange.x;
        valueRange.y = (defaultRange.y - defaultRange.x) * value_range_percentage.y * 0.01f + defaultRange.x;
      }
    }
  }

  ImGui::EndGroup();

  //------------ Transfer Function Editor -------------

  ImGui::Spacing();
  draw_tfn_editor(11.f, ImGui::GetContentRegionAvail().y - 60.f);

  //------------ End Transfer Function Editor ---------
}

inline void renderTFNTexture(GLuint &tex, int width, int height)
{
  GLint prev_binding = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_binding);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (prev_binding) {
    glBindTexture(GL_TEXTURE_2D, prev_binding);
  }
}

inline void TransferFunctionWidget::render(int tfn_w, int tfn_h)
{
  // Upload to GL if the transfer function has changed
  if (!tfn_palette) {
    renderTFNTexture(tfn_palette, tfn_w, tfn_h);
  } else {
    /* ... */
  }

  // Update texture color
  if (tfn_changed) {
    // Backup old states
    GLint prev_binding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_binding);

    // Sample the palette then upload the data
    std::vector<uint8_t> palette(tfn_w * tfn_h * 4, 0);
    std::vector<float> colors(3 * tfn_w, 1.f);
    std::vector<float> alpha(2 * tfn_w, 1.f);
    const float step = 1.0f / (float)(tfn_w - 1);
    for (int i = 0; i < tfn_w; ++i) {
      const float p = clamp(i * step, 0.0f, 1.0f);
      int ir, il;
      /* color */
      {
        std::tie(il, ir) = find_interval(tfn_c, p);
        float pl = tfn_c->at(il).p;
        float pr = tfn_c->at(ir).p;
        const float r = lerp(tfn_c->at(il).r, tfn_c->at(ir).r, pl, pr, p);
        const float g = lerp(tfn_c->at(il).g, tfn_c->at(ir).g, pl, pr, p);
        const float b = lerp(tfn_c->at(il).b, tfn_c->at(ir).b, pl, pr, p);
        colors[3 * i + 0] = r;
        colors[3 * i + 1] = g;
        colors[3 * i + 2] = b;
        /* palette */
        palette[i * 4 + 0] = static_cast<uint8_t>(r * 255.f);
        palette[i * 4 + 1] = static_cast<uint8_t>(g * 255.f);
        palette[i * 4 + 2] = static_cast<uint8_t>(b * 255.f);
        palette[i * 4 + 3] = 255;
      }
      /* opacity */
      {
        std::tie(il, ir) = find_interval(tfn_o, p);
        float pl = tfn_o->at(il).p;
        float pr = tfn_o->at(ir).p;
        const float a = lerp(tfn_o->at(il).a, tfn_o->at(ir).a, pl, pr, p);
        alpha[2 * i + 0] = p;
        alpha[2 * i + 1] = a;
      }
    }

    // Render palette again
    glBindTexture(GL_TEXTURE_2D, tfn_palette);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        tfn_w,
        tfn_h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        static_cast<const void *>(palette.data())); // We need to resize texture of texture is resized
    if (prev_binding) { // Restore previous binded texture
      glBindTexture(GL_TEXTURE_2D, prev_binding);
    }

    this->_setter_cb(colors, alpha, valueRange);
    tfn_changed = false;
  }
}

inline void TransferFunctionWidget::load(const std::string &fileName)
{
  tfn::TransferFunction loaded(fileName);
  tfn_readers.emplace_back(fileName);
  const auto tfn_new = tfn_readers.back();
  const int c_size = (int)tfn_new.rgbValues.size();
  const int o_size = (int)tfn_new.opacityValues.size();
  // load data
  tfn_c_list.emplace_back(c_size);
  tfn_o_list.emplace_back(o_size);
  tfn_editable.push_back(false); // TODO we dont want to edit loaded TFN
  tfn_names.push_back(tfn_new.name);
  set_tfn((int)(tfn_names.size() - 1)); // set the loaded function as current
  if (c_size < 2) {
    throw std::runtime_error(
        "transfer function contains too "
        "few color points");
  }
  const float c_step = 1.f / (c_size - 1);
  for (int i = 0; i < c_size; ++i) {
    const float p = static_cast<float>(i) * c_step;
    (*tfn_c)[i].p = p;
    (*tfn_c)[i].r = tfn_new.rgbValues[i].x;
    (*tfn_c)[i].g = tfn_new.rgbValues[i].y;
    (*tfn_c)[i].b = tfn_new.rgbValues[i].z;
  }
  if (o_size < 2) {
    throw std::runtime_error(
        "transfer function contains too "
        "few opacity points");
  }
  for (int i = 0; i < o_size; ++i) {
    (*tfn_o)[i].p = tfn_new.opacityValues[i].x;
    (*tfn_o)[i].a = tfn_new.opacityValues[i].y;
  }
  tfn_changed = true;
}

inline void tfn::TransferFunctionWidget::save(const std::string &fileName) const
{
  // // For opacity we can store the associated data value and only have 1 line,
  // // so just save it out directly
  // tfn::TransferFunction output(transferFunctions[tfcnSelection].name,
  //     std::vector<vec3f>(), rgbaLines[3].line, 0, 1, 1);

  // // Pull the RGB line values to compute the transfer function and save it
  // out
  // // here we may need to do some interpolation, if the RGB lines have
  // differing numbers
  // // of control points
  // // Find which x values we need to sample to get all the control points for
  // the tfcn. std::vector<float> controlPoints; for (size_t i = 0; i < 3; ++i)
  // {
  //   for (const auto &x : rgbaLines[i].line)
  //     controlPoints.push_back(x.x);
  // }

  // // Filter out same or within epsilon control points to get unique list
  // std::sort(controlPoints.begin(), controlPoints.end());
  // auto uniqueEnd = std::unique(controlPoints.begin(), controlPoints.end(),
  //     [](const float &a, const float &b) { return std::abs(a - b) < 0.0001;
  //     });
  // controlPoints.erase(uniqueEnd, controlPoints.end());

  // // Step along the lines and sample them
  // std::array<std::vector<vec2f>::const_iterator, 3> lit = {
  //   rgbaLines[0].line.begin(), rgbaLines[1].line.begin(),
  //   rgbaLines[2].line.begin()
  // };

  // for (const auto &x : controlPoints) {
  //   std::array<float, 3> sampleColor;
  //   for (size_t j = 0; j < 3; ++j) {
  //     if (x > (lit[j] + 1)->x)
  //       ++lit[j];

  //     assert(lit[j] != rgbaLines[j].line.end());
  //     const float t = (x - lit[j]->x) / ((lit[j] + 1)->x - lit[j]->x);
  //     // It's hard to click down at exactly 0, so offset a little bit
  //     sampleColor[j] = clamp(lerp(lit[j]->y - 0.001, (lit[j] + 1)->y - 0.001,
  //     t));
  //   }
  //   output.rgbValues.push_back(vec3f(sampleColor[0], sampleColor[1],
  //   sampleColor[2]));
  // }

  // output.save(fileName);
  ;
}

inline void TransferFunctionWidget::set_default_tfns()
{
  for (auto &ct : _predef_color_table_) {
    tfn_c_list.emplace_back(ct.second.size() / 4);
    for (size_t i = 0; i < ct.second.size() / 4; ++i) {
      tfn_c_list.back()[i] = ColorPoint(ct.second[i * 4], ct.second[i * 4 + 1], ct.second[i * 4 + 2], ct.second[i * 4 + 3]);
    }
    tfn_o_list.emplace_back(5);
    tfn_o_list.back()[0] = OpacityPoint_Linear(0.00f, 0.00f);
    tfn_o_list.back()[1] = OpacityPoint_Linear(0.25f, 0.25f);
    tfn_o_list.back()[2] = OpacityPoint_Linear(0.50f, 0.50f);
    tfn_o_list.back()[3] = OpacityPoint_Linear(0.75f, 0.75f);
    tfn_o_list.back()[4] = OpacityPoint_Linear(1.00f, 1.00f);
    tfn_editable.push_back(ct.second.size() < 40);
    tfn_names.push_back(ct.first);
  }
};

} // namespace tfn
