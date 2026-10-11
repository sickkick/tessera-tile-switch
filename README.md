# Switch on the tile

A tile for a light, a switch, a fan or anything else that turns on and off. It shows the icon, the name and On or Off, with the switch along the bottom. A tap turns it on or off.

This is a tile of its own. It does not change Tessera's own light and switch tiles.

## Set up

1. Add the plugin to a screen in Tessera. The screen builds once.
2. In Layout, place **Switch** from the library's Plugins group.
3. In the inspector, choose the entity. A light, switch, input boolean, fan, automation, remote or humidifier.
4. Icon picks the picture at the top left. Automatic follows the entity. Drag an edge to change the size, up to four columns wide and two rows high. Two columns uses the larger title. Two rows lets the name wrap onto a second line. The switch stays a short pill in the bottom right.

## Good to know

- The switch uses the same colours as the switch on Tessera's wider tiles: blue while on, grey while off.
- A tap anywhere on the tile toggles it. The plugin draws the switch, and the tile has one tap.
- While Home Assistant has not answered yet, the tile says Unavailable and a tap does nothing.
