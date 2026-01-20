#include <raylib.h>
#define CLAY_IMPLEMENTATION
#include "../clay.h"
#include "../clay_renderer_raylib.c"
#include "../game/game_object.h"
#include <iostream>

#define RAYLIB_VECTOR2_TO_CLAY_VECTOR2(vector)                                 \
  (Clay_Vector2) { .x = vector.x, .y = vector.y }
void HandleClayErrors(Clay_ErrorData errorData) {
  std::cout << errorData.errorText.chars << "\n";
}

typedef struct {
  Clay_Vector2 clickOrigin;
  Clay_Vector2 positionOrigin;
  bool mouseDown;
} ScrollbarData;
Clay_RenderCommandArray CreateLayout(void) {
  Clay_BeginLayout();
  CLAY(CLAY_ID("OuterContainer"),
       {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                              .height = CLAY_SIZING_GROW(0)},
                   .padding = {16, 16, 16, 16},
                   .childGap = 16},
        .backgroundColor = {200, 200, 200, 255}}) {
    CLAY(CLAY_ID("SideBar"),
         {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {.width = CLAY_SIZING_FIXED(300),
                                .height = CLAY_SIZING_GROW(0)},
                     .padding = {16, 16, 16, 16},
                     .childGap = 16},
          .backgroundColor = {150, 150, 255, 255}}) {}
    CLAY(CLAY_ID("SidebarBlob1"),
         {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {110, 110, 255, 255}}) {}
    CLAY(CLAY_ID("SidebarBlob2"),
         {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {110, 110, 255, 255}}) {}
    CLAY(CLAY_ID("SidebarBlob3"),
         {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {110, 110, 255, 255}}) {}
    CLAY(CLAY_ID("SidebarBlob4"),
         {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0),
                                .height = CLAY_SIZING_FIXED(50)}},
          .backgroundColor = {110, 110, 255, 255}}) {}
  }
}
class Gui : public moiras::GameObject {
private:
  uint64_t clayRequiredMemory;
  Clay_Arena clayMemory;
  bool debugMode = true;
  ScrollbarData scrollbarData;

public:
  Font *fonts;
  Clay_RenderCommand renderCommands;
  Gui() {
    clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(
        clayRequiredMemory, malloc(clayRequiredMemory));
    Clay_Initialize(
        clayMemory,
        (Clay_Dimensions){(float)GetScreenWidth(), (float)GetScreenHeight()},
        (Clay_ErrorHandler){HandleClayErrors, 0});
    ScrollbarData scrollbarData = {0};
  }
  ~Gui() { Clay_Raylib_Close(); }
  void update() override {
    Vector2 mouseWheelDelta = GetMouseWheelMoveV();
    float mouseWheelX = mouseWheelDelta.x;
    float mouseWheelY = mouseWheelDelta.y;
    if (IsKeyPressed(KEY_D)) {
      debugMode = !debugMode;
      Clay_SetDebugModeEnabled(debugMode);
    }
    //----------------------------------------------------------------------------------
    // Handle scroll containers
    Clay_Vector2 mousePosition =
        RAYLIB_VECTOR2_TO_CLAY_VECTOR2(GetMousePosition());
    Clay_SetPointerState(mousePosition,
                         IsMouseButtonDown(0) && !scrollbarData.mouseDown);
    Clay_SetLayoutDimensions(
        (Clay_Dimensions){(float)GetScreenWidth(), (float)GetScreenHeight()});
    if (!IsMouseButtonDown(0)) {
      scrollbarData.mouseDown = false;
    }

    if (IsMouseButtonDown(0) && !scrollbarData.mouseDown &&
        Clay_PointerOver(Clay_GetElementId(CLAY_STRING("ScrollBar")))) {
      Clay_ScrollContainerData scrollContainerData =
          Clay_GetScrollContainerData(
              Clay_GetElementId(CLAY_STRING("MainContent")));
      scrollbarData.clickOrigin = mousePosition;
      scrollbarData.positionOrigin = *scrollContainerData.scrollPosition;
      scrollbarData.mouseDown = true;
    } else if (scrollbarData.mouseDown) {
      Clay_ScrollContainerData scrollContainerData =
          Clay_GetScrollContainerData(
              Clay_GetElementId(CLAY_STRING("MainContent")));
      if (scrollContainerData.contentDimensions.height > 0) {
        Clay_Vector2 ratio = (Clay_Vector2){
            scrollContainerData.contentDimensions.width /
                scrollContainerData.scrollContainerDimensions.width,
            scrollContainerData.contentDimensions.height /
                scrollContainerData.scrollContainerDimensions.height,
        };
        if (scrollContainerData.config.vertical) {
          scrollContainerData.scrollPosition->y =
              scrollbarData.positionOrigin.y +
              (scrollbarData.clickOrigin.y - mousePosition.y) * ratio.y;
        }
        if (scrollContainerData.config.horizontal) {
          scrollContainerData.scrollPosition->x =
              scrollbarData.positionOrigin.x +
              (scrollbarData.clickOrigin.x - mousePosition.x) * ratio.x;
        }
      }
    }

    Clay_UpdateScrollContainers(true, (Clay_Vector2){mouseWheelX, mouseWheelY},
                                GetFrameTime());
    // Generate the auto layout for rendering
    double currentTime = GetTime();
    Clay_RenderCommandArray renderCommands = CreateLayout();
  };
  void draw() override { Clay_Raylib_Render(renderCommands, *fonts); };
};
