// Kameo - ReXGlue Recompiled Project
//
// Minimal ReXApp subclass. The SDK's ReXApp base wires up runtime, window,
// ImGui overlays, and module launch automatically. Override the protected
// virtual hooks (OnPreSetup, OnPostSetup, etc.) to customize behavior.

#include "generated/Kameo_init.h"

#include <rex/rex_app.h>

class KameoApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<KameoApp>(new KameoApp(ctx, "Kameo", PPCImageConfig));
  }
};

REX_DEFINE_APP(Kameo, KameoApp::Create)
