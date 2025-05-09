# mindmap
A simple yet fast mindmap editor written in C++. You can also call LLM to get some inspiration (if you run LLM locally with Ollama).

# features
* GUI style dark/light/classic
* Interactive animation
* Manual layout with guidline
* Auto-Layout and Auto-route.
* Canvas zoom-in-out-pan
* Graph editing redo/undo
* Import/Export edit result as json file (image data encoded as bas64 string)
* Export graph as svg image format or html format
* Use template engine to generate svg, html export, so you can customize your own output format by modifying template files.
* Call LLM to generate context related ideas.
* You can modify prompt(for calling LLM) template as your wish.
* You can select which LLM model to call by right click "ai." button.

[![Watch the video](https://raw.githubusercontent.com/azula1A89/mindmap/main/docs/pictures/thumbnail.png)](https://github.com/user-attachments/assets/be4c39d2-5789-451f-b4c7-4e02d0903eb0)

# Tips
  * Double-click blank aera to create a node.
  * Click on blank area to deselect.
  * Click on node to select node.
  * Click on edge to select edge.
  * Click on group to select group.
  * Hold CTRL + click to select continuously.
  * Node(s) selected + press Shift + click on node to create edge(s).
  * Node(s) selected + press Alt to create group.
  * Draw selection box from a blank area on the canvas to select all nodes in the box.
  * Drag on node to move node.
  * Drag on group to move group.
  * Double click object to edit it's label.
  * CTRL + A to select all.
  * CTRL + C to copy.
  * CTRL + V to paste.
  * The Copy command copies only the selected nodes and the edges between them, if any.
  * CTRL + Z to undo.
  * CTRL + Y to redo.
  * Use locking if you do not want the auto-layout algorithm to change the position of a node or group.
  * You can remove bending points on an edge by right-clicking the item in the edge table then click "reset" in the context menu.


# Build Environment
Only [Windows-MSYS2-UCRT64](https://www.msys2.org/docs/environments/) supported(currently). Try [binary](https://github.com/azula1A89/mindmap/releases/download/tagv1.0.0/bin.zip).
I am not familiar with the Linux environment, so I have only compiled and tested it under the Windows operating system.

## How to build

After installing the environment, open the MSYS2-UCRT64 shell and execute the following command:
```shell
git clone https://github.com/azula1A89/mindmap.git
cd mindmap
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

# third-party code
  * base64: https://github.com/tobiaslocker/base64
  * fmt: https://github.com/fmtlib/fmt
  * glew: https://glew.sourceforge.net/
  * glfw: https://www.glfw.org/
  * httplib: https://github.com/yhirose/cpp-httplib
  * imgui: https://github.com/ocornut/imgui
  * imgui_markdown https://github.com/juliettef/imgui_markdown
  * imgui-node-editor https://github.com/thedmd/imgui-node-editor
  * inja: https://github.com/pantor/inja
  * json: https://github.com/nlohmann/json
  * adaptagrams: https://github.com/mjwybrow/adaptagrams
  * Native File Dialog: https://github.com/btzy/nativefiledialog-extended
  * stb_image: http://nothings.org/stb
