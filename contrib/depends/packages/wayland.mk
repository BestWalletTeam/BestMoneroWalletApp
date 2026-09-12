package=wayland
$(package)_version=1.25.0
$(package)_download_path := https://gitlab.freedesktop.org/wayland/wayland/-/archive/$($(package)_version)/
$(package)_file_name := wayland-$($(package)_version).tar.gz
$(package)_sha256_hash := 95413723f6cd0462770eda207a1c78d142d0a1bba9b0572a4ed1da6a9b27059c
$(package)_dependencies=native_wayland libffi expat
$(package)_patches = toolchain.txt

define $(package)_preprocess_cmds
  cp $($(package)_patch_dir)/toolchain.txt toolchain.txt && \
  sed -i -e 's|@host_prefix@|$(host_prefix)|' \
         -e 's|@build_prefix@|$(build_prefix)|' \
         -e 's|@cc@|$($(package)_cc)|' \
         -e 's|@cxx@|$($(package)_cxx)|' \
         -e 's|@ar@|$($(package)_ar)|' \
         -e 's|@strip@|$(host_STRIP)|' \
         -e 's|@arch@|$(host_arch)|' \
	  toolchain.txt
endef

define $(package)_config_cmds
  PKG_CONFIG_LIBDIR="$(host_prefix)/lib/pkgconfig:$(build_prefix)/lib/pkgconfig" PKG_CONFIG_PATH="$(build_prefix)/lib/pkgconfig" meson setup --cross-file toolchain.txt build -Ddtd_validation=false -Ddocumentation=false -Dprefer_static=true -Ddefault_library=static -Dtests=false -Dscanner=false
endef

define $(package)_build_cmds
  ninja -C build
endef

define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) ninja -C build install
endef
