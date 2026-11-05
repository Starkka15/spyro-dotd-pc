// spyro_dotd - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#include <rex/filesystem.h>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/gpu_plugin.h>

// Which graphics API the xenos plugin uses: "d3d12", "vulkan" or "any" (D3D12 first on Windows).
// On SteamOS/Proton, "vulkan" skips the D3D12-to-Vulkan layer (vkd3d).
REXCVAR_DEFINE_STRING(gpu_backend, "any", "GPU", "Graphics API for the xenos GPU plugin: any, d3d12, vulkan")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

class SpyroDotdApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<SpyroDotdApp>(new SpyroDotdApp(ctx, "spyro_dotd",
        PPCImageConfig));
  }

  // No --game_data_root: look next to the exe for a folder holding default.xex, so an installed
  // copy (exe + game/) runs without arguments. Checked in order: game, game/files, game_files, the
  // exe folder itself. The name match ignores case (the disc has Default.xex).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (!paths.game_data_root.empty()) return;
    std::error_code ec;
    auto exe_dir = rex::filesystem::GetExecutableFolder();
    auto has_xex = [&](const std::filesystem::path& dir) {
      if (!std::filesystem::is_directory(dir, ec)) return false;
      for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        std::string name = entry.path().filename().string();
        for (auto& c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
        if (name == "default.xex" && entry.is_regular_file(ec)) return true;
      }
      return false;
    };
    for (const auto& dir : {exe_dir / "game", exe_dir / "game" / "files", exe_dir / "game_files", exe_dir}) {
      if (has_xex(dir)) {
        paths.game_data_root = dir;
        return;
      }
    }
  }

  // The SDK leaves GPU emulation off unless gpu_plugin is set; this game needs it.
  void OnPreSetup(rex::RuntimeConfig& config) override {
    std::string plugin = config.gpu_plugin.empty() ? std::string("xenos") : config.gpu_plugin;
    std::string backend = REXCVAR_GET(gpu_backend);
    REXLOG_INFO("spyro_dotd: GPU plugin '{}', backend '{}'", plugin, backend);
    config.gpu_plugin = plugin;
    config.graphics = rex::system::LoadGpuPlugin(plugin, backend);
  }

  // The game's name in the title bar instead of the SDK build stamp.
  void OnPreLaunchModule() override {
    if (window()) window()->SetTitle("The Legend of Spyro: Dawn of the Dragon");
  }

  // Dev aid: SPYRO_DUMP_IMAGE=<file> writes the loaded, decompressed XEX image
  // (0x82000000-0x828E0000) so tools/find_funcs.py can look for missed functions.
  void OnPostLoadXexImage() override {
    const char* path = std::getenv("SPYRO_DUMP_IMAGE");
    if (!path || !*path) return;
    const uint8_t* img = runtime()->virtual_membase() + 0x82000000u;
    if (FILE* f = std::fopen(path, "wb")) {
      size_t n = std::fwrite(img, 1, 0x8E0000, f);
      std::fclose(f);
      REXLOG_INFO("spyro_dotd: dumped {} image bytes to {}", n, path);
    }
  }
};
