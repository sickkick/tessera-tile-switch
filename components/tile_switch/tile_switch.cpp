#include "tile_switch.h"

#include <cstring>
#include <string>

namespace esphome::tile_switch {

using tessera::Font;
namespace ui = tessera::ui;

// Icon of Tessera's set for the entity's domain (tile_icon_names.h).
static uint32_t icon_of(const std::string &domain) {
  if (domain == "light") return 0xF0335;          // lightbulb
  if (domain == "fan") return 0xF0210;            // fan
  if (domain == "humidifier") return 0xF1099;     // air-humidifier
  if (domain == "automation") return 0xF0BC2;     // script-text
  if (domain == "remote") return 0xF0454;         // remote
  return 0xF0521;                                 // toggle-switch
}

// A row: icon, name, On or Off, and the switch at the right. A tap toggles the entity.
class SwitchTile : public tessera::Tile {
 public:
  explicit SwitchTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    entity_ = c.entity ? c.entity : "";
    const auto dot = entity_.find('.');
    domain_ = dot == std::string::npos ? std::string() : entity_.substr(0, dot);
    chosen_name_ = c.name ? c.name : "";

    icon_ = ui::label(c.parent, Font::ICON, theme::OFF);
    ui::set_text(icon_, ui::icon(icon_of(domain_)));
    name_ = ui::label(c.parent, Font::TITLE, theme::INK);
    state_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    track_ = ui::block(c.parent, theme::PANEL_TOGGLE_OFF);
    knob_ = ui::block(track_, theme::KNOB);
    lv_obj_set_style_radius(track_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_radius(knob_, LV_RADIUS_CIRCLE, 0);
    place();
    paint();
  }

  void on_state(JsonObjectConst data) override {
    if (data["wait"].is<const char *>()) {
      known_ = false;
      on_ = false;
    } else {
      const char *state = data["state"] | "";
      on_ = std::strcmp(state, "on") == 0;
      known_ = on_ || std::strcmp(state, "off") == 0;
    }
    const char *reported = data["name"] | "";
    reported_name_ = reported;
    place_knob();
    paint();
  }

  void on_theme() override { paint(); }

  void on_tap() override {
    if (entity_.empty() || domain_.empty() || !known_) return;
    on_ = !on_;
    known_ = true;
    place_knob();
    paint();
    const std::string service = domain_ + ".toggle";
    tessera::action(service.c_str(), entity_);
  }

 private:
  void place() {
    const int pad = ui::px(4);
    const int gap = ui::px(6);
    const int icon_h = std::max(1, ui::line_height(Font::ICON));
    track_h_ = std::max(ui::mm(7), icon_h);
    track_w_ = track_h_ * 2;
    if (track_w_ > width_ / 3) track_w_ = std::max(track_h_ + ui::px(8), width_ / 3);
    const int row_h = std::max(icon_h, track_h_);
    const int top = std::max(0, (height_ - row_h) / 2);
    lv_obj_set_pos(icon_, pad, top + (row_h - icon_h) / 2);

    const int switch_x = std::max(pad, width_ - pad - track_w_);
    lv_obj_set_pos(track_, switch_x, top + (row_h - track_h_) / 2);
    lv_obj_set_size(track_, track_w_, track_h_);
    knob_size_ = std::max(4, track_h_ - ui::px(4));
    knob_pad_ = std::max(0, (track_h_ - knob_size_) / 2);
    lv_obj_set_size(knob_, knob_size_, knob_size_);
    place_knob();

    const int name_h = std::max(1, ui::line_height(Font::TITLE));
    const int state_h = std::max(1, ui::line_height(Font::BODY));
    const int text_x = pad + icon_h + gap;
    const int text_w = std::max(1, switch_x - gap - text_x);
    const int text_y = std::max(0, top + (row_h - name_h - state_h) / 2);
    lv_obj_set_pos(name_, text_x, text_y);
    lv_obj_set_size(name_, text_w, name_h);
    lv_obj_set_pos(state_, text_x, text_y + name_h);
    lv_obj_set_size(state_, text_w, state_h);
  }

  void place_knob() {
    if (!knob_) return;
    lv_obj_set_pos(knob_, on_ ? track_w_ - knob_size_ - knob_pad_ : knob_pad_, knob_pad_);
  }

  void paint() {
    const std::string name = !chosen_name_.empty() ? chosen_name_ : reported_name_;
    ui::set_text(name_, name.empty() ? plugin_->text("choose") : name);
    const char *word = !known_ ? "unavailable" : on_ ? "on" : "off";
    ui::set_text(state_, plugin_->text(word));
    ui::set_color(icon_, known_ && on_ ? theme::ACCENT_ICON : theme::OFF);
    ui::set_color(name_, theme::INK);
    ui::set_color(state_, theme::MUTED);
    if (track_) lv_obj_set_style_bg_color(track_, ui::color(known_ && on_ ? theme::ACCENT : theme::PANEL_TOGGLE_OFF), 0);
    if (knob_) lv_obj_set_style_bg_color(knob_, ui::color(theme::KNOB), 0);
  }

  const tessera::Plugin *plugin_;
  std::string entity_, domain_, chosen_name_, reported_name_;
  bool on_ = false, known_ = false;
  int width_ = 0, height_ = 0, track_w_ = 0, track_h_ = 0, knob_size_ = 0, knob_pad_ = 0;
  lv_obj_t *icon_{}, *name_{}, *state_{}, *track_{}, *knob_{};
};

void TileSwitch::setup() {
  add_tile("toggle", [this]() { return new SwitchTile(this); });
}

}  // namespace esphome::tile_switch
