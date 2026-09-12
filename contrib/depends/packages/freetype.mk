package=freetype
$(package)_version=2.14.1
$(package)_download_path=https://sourceforge.net/projects/freetype/files/freetype2/$($(package)_version)/
$(package)_file_name=freetype-$($(package)_version).tar.gz
$(package)_sha256_hash=174d9e53402e1bf9ec7277e22ec199ba3e55a6be2c0740cb18c0ee9850fc8c34

define $(package)_set_vars
  $(package)_config_opts := --without-zlib --without-png --without-harfbuzz --without-bzip2 --enable-static --disable-shared
  $(package)_config_opts += --enable-option-checking --without-brotli
  $(package)_config_opts += --with-pic
endef

define $(package)_preprocess_cmds
  rm -rf docs
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share/man lib/*.la
endef
