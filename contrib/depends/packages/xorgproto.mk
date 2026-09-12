package=xorgproto
$(package)_version=2025.1
$(package)_download_path=https://xorg.freedesktop.org/archive/individual/proto
$(package)_file_name=xorgproto-$($(package)_version).tar.gz
$(package)_sha256_hash=d6f89f65bafb8c9b735e0515882b8a1511e8e864dde5e9513e191629369f2256

define $(package)_set_vars
  $(package)_config_opts=--without-fop --without-xmlto --without-xsltproc --disable-specs
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
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
