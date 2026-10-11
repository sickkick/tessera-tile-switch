#include "tile_switch.h"

#include <cstring>
#include <string>

namespace esphome::tile_switch {

using tessera::Font;
namespace ui = tessera::ui;

// Icon of Tessera's set for the entity's domain, used when the tile's Icon option is Automatic.
static uint32_t icon_of(const std::string &domain) {
  if (domain == "light") return 0xF0335;          // lightbulb
  if (domain == "fan") return 0xF0210;            // fan
  if (domain == "humidifier") return 0xF1099;     // air-humidifier
  if (domain == "automation") return 0xF0BC2;     // script-text
  if (domain == "remote") return 0xF0454;         // remote
  return 0xF0521;                                 // toggle-switch
}

// The Icon option's choices, by Material Design Icons name. The same codepoints as Tessera's icon set.
struct IconChoice { const char *name; uint32_t codepoint; };
static constexpr IconChoice ICON_CHOICES[] = {
  {"lightbulb", 0xF0335},
  {"lightbulb-group", 0xF1253},
  {"ceiling-light", 0xF0769},
  {"ceiling-fan", 0xF1797},
  {"fan", 0xF0210},
  {"lamp", 0xF06B5},
  {"floor-lamp", 0xF08DD},
  {"desk-lamp", 0xF095F},
  {"chandelier", 0xF1793},
  {"spotlight-beam", 0xF04C9},
  {"led-strip-variant", 0xF1051},
  {"outdoor-lamp", 0xF1054},
  {"toggle-switch", 0xF0521},
  {"power", 0xF0425},
  {"power-plug", 0xF06A5},
  {"air-humidifier", 0xF1099},
  {"air-purifier", 0xF0D44},
  {"script-text", 0xF0BC2},
  {"remote", 0xF0454},
};

static uint32_t icon_picked(const char *name, const std::string &domain) {
  if (name && name[0] && std::strcmp(name, "auto") != 0)
    for (const auto &choice : ICON_CHOICES)
      if (std::strcmp(name, choice.name) == 0) return choice.codepoint;
  return icon_of(domain);
}

// Icon, name and On or Off at the top, and the switch as a pill along the bottom. A tap toggles the entity.
class SwitchTile : public tessera::Tile {
 public:
  explicit SwitchTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    columns_ = c.columns ? c.columns : 1;
    rows_ = c.rows ? c.rows : 1;
    entity_ = c.entity ? c.entity : "";
    const auto dot = entity_.find('.');
    domain_ = dot == std::string::npos ? std::string() : entity_.substr(0, dot);
    chosen_name_ = c.name ? c.name : "";

    icon_ = ui::label(c.parent, Font::ICON, theme::OFF);
    ui::set_text(icon_, ui::icon(icon_picked(c.options["icon"] | "auto", domain_)));
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
    const int gap = ui::px(3);
    const int icon_h = std::max(1, ui::line_height(Font::ICON));
    const int state_h = std::max(1, ui::line_height(Font::BODY));

    // A short pill in the bottom right. Its width stays put when the tile grows, so it does not become a bar.
    track_h_ = ui::px(16);
    const int max_h = std::max(ui::px(12), height_ / 5);
    if (track_h_ > max_h) track_h_ = max_h;
    track_w_ = track_h_ * 5 / 2;
    const int widest = std::max(track_h_ * 2, (width_ - 2 * pad) / 2);
    if (track_w_ > widest) track_w_ = widest;
    const int switch_x = std::max(pad, width_ - pad - track_w_);
    const int switch_y = std::max(pad, height_ - pad - track_h_);
    lv_obj_set_pos(track_, switch_x, switch_y);
    lv_obj_set_size(track_, track_w_, track_h_);
    knob_pad_ = std::max(1, ui::px(2));
    knob_size_ = std::max(4, track_h_ - 2 * knob_pad_);
    lv_obj_set_size(knob_, knob_size_, knob_size_);
    place_knob();

    // A compact screen keeps a one-cell name small. Two columns is the room Tessera uses for the larger name.
    Font name_font = Font::TITLE;
    if (columns_ >= 2 && !ui::large()) {
      const int big = ui::line_height(Font::BODY_LARGE);
      const int below = height_ - pad - track_h_ - ui::px(4);
      if (big > 0 && big + state_h <= below) name_font = Font::BODY_LARGE;
    }
    ui::set_font(name_, name_font);
    const int name_h = std::max(1, ui::line_height(name_font));
    // One row keeps a single line. Two rows gives the name a second line, when that still sits above the switch.
    int name_lines = 1;
    if (rows_ >= 2 && pad + name_h * 2 + state_h + gap <= switch_y) name_lines = 2;

    const int text_x = pad + icon_h + gap;
    const int text_w = std::max(1, width_ - pad - text_x);
    const int name_box = name_h * name_lines;
    const int block = name_box + state_h;
    const int text_y = pad;
    lv_label_set_long_mode(name_, name_lines > 1 ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_DOT);
    lv_obj_set_pos(icon_, pad, text_y + std::max(0, (name_lines > 1 ? name_h : block) - icon_h) / 2);
    lv_obj_set_pos(name_, text_x, text_y);
    lv_obj_set_size(name_, text_w, name_box);
    lv_obj_set_pos(state_, text_x, text_y + name_box);
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
  int width_ = 0, height_ = 0, columns_ = 1, rows_ = 1, track_w_ = 0, track_h_ = 0, knob_size_ = 0, knob_pad_ = 0;
  lv_obj_t *icon_{}, *name_{}, *state_{}, *track_{}, *knob_{};
};

void TileSwitch::setup() {
  add_tile("toggle", [this]() { return new SwitchTile(this); });
}

}  // namespace esphome::tile_switch
