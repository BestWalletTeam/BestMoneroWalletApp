package=libsodium
$(package)_version=1.0.22
$(package)_download_path=https://download.libsodium.org/libsodium/releases/
$(package)_file_name=libsodium-$($(package)_version).tar.gz
$(package)_sha256_hash=adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349
$(package)_patches=fix-blake2b-symbol-naming.patch

define $(package)_set_vars
  $(package)_config_opts=--enable-static --disable-shared
  $(package)_config_opts+=--prefix=$(host_prefix)
endef

define $(package)_preprocess_cmds
  rm -rf builds/msvc &&\
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub build-aux/ && \
  patch -p1 -i $($(package)_patch_dir)/fix-blake2b-symbol-naming.patch
endef

define $(package)_config_cmds
  $($(package)_autoconf) AR_FLAGS=$($(package)_arflags)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
