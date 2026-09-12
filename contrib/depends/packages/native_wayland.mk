package=native_wayland
$(package)_version=1.25.0
$(package)_download_path := https://gitlab.freedesktop.org/wayland/wayland/-/archive/$($(package)_version)/
$(package)_file_name := wayland-$($(package)_version).tar.gz
$(package)_sha256_hash := 95413723f6cd0462770eda207a1c78d142d0a1bba9b0572a4ed1da6a9b27059c
$(package)_dependencies := native_expat native_libffi

define $(package)_config_cmds
  meson setup build -Dprefix="$(build_prefix)" -Ddtd_validation=false -Ddocumentation=false -Dtests=false
endef

define $(package)_build_cmds
  ninja -C build
endef

define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) ninja -C build install
endef
