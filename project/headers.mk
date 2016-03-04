API_HEADERS = \
	$(PROJECT_DIR)/include/$(PACKAGE)/slibtool.h \
	$(PROJECT_DIR)/include/$(PACKAGE)/slibtool_api.h \

INTERNAL_HEADERS = \
	$(PROJECT_DIR)/src/internal/argv/argv.h \
	$(PROJECT_DIR)/src/internal/$(PACKAGE)_driver_impl.h \

ALL_HEADERS = $(API_HEADERS) $(INTERNAL_HEADERS)