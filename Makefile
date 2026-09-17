CC ?= cc
PKG_CONFIG ?= pkg-config
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
DESTDIR ?=
PROGRAM ?= geome
VERSION ?= 1.0.0
CFLAGS ?= -O2 -g
HARDEN ?= 1
WARNINGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes
PROJECT_CPPFLAGS = -Iinclude $(shell $(PKG_CONFIG) --cflags libcurl libcjson) -DGEOME_VERSION='"$(VERSION)"' -DGEOME_PROGRAM='"$(PROGRAM)"'
PROJECT_LIBS = $(shell $(PKG_CONFIG) --libs libcurl libcjson) -lm
comma := ,
cc_option = $(shell $(CC) -Werror $(1) -x c -c /dev/null -o /dev/null >/dev/null 2>&1 && printf '%s' '$(1)')
ld_option = $(shell $(CC) -Werror $(1) tests/compiler_probe.c -o /dev/null >/dev/null 2>&1 && printf '%s' '$(1)')
ifeq ($(HARDEN),1)
HARDEN_CFLAGS := $(call cc_option,-fstack-protector-strong) $(call cc_option,-fPIE)
HARDEN_LDFLAGS := $(call ld_option,-Wl$(comma)-z$(comma)relro) $(call ld_option,-Wl$(comma)-z$(comma)now) $(call ld_option,-pie)
PROJECT_CPPFLAGS += -D_FORTIFY_SOURCE=2
endif
COMPILE_FLAGS = $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(HARDEN_CFLAGS)
LINK_FLAGS = $(LDFLAGS) $(HARDEN_LDFLAGS)
LIBRARIES = $(LDLIBS) $(PROJECT_LIBS)

.DEFAULT_GOAL := all
SOURCES = main cli http_client location output
OBJECTS = $(addprefix build/src/,$(addsuffix .o,$(SOURCES)))

.PHONY: all release debug test integration-test check sanitize install uninstall clean dist FORCE
all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(LINK_FLAGS) $^ $(LIBRARIES) -o $@

release:
	$(MAKE) all CFLAGS="$(CFLAGS) -O2" HARDEN=1

debug:
	$(MAKE) all CFLAGS="$(CFLAGS) -O0 -g3" HARDEN=0

test: build/test_location build/test_output build/test_http
	./build/test_location
	./build/test_output
	./build/test_http

integration-test: all build/geome-fixture build/validate_json
	GEOME_TEST_PROGRAM=./$(PROGRAM) GEOME_TEST_VERSION=$(VERSION) sh tests/integration/test_cli.sh

check: test integration-test

FAULT_LINK = -Wl,--wrap=malloc -Wl,--wrap=realloc

build/test_location: build/tests/test_location.o build/src/location.o build/tests/fault_alloc.o
	$(CC) $(LINK_FLAGS) $(FAULT_LINK) $^ $(LIBRARIES) -o $@

build/test_output: build/tests/test_output.o build/src/output.o build/src/location.o build/tests/fault_alloc.o
	$(CC) $(LINK_FLAGS) $(FAULT_LINK) $^ $(LIBRARIES) -o $@

HTTP_WRAPS = curl_easy_setopt curl_easy_perform curl_easy_getinfo curl_easy_init curl_slist_append
build/test_http: build/tests/test_http.o build/src/http_client.o build/tests/fault_alloc.o
	$(CC) $(LINK_FLAGS) $(FAULT_LINK) $(foreach symbol,$(HTTP_WRAPS),-Wl,--wrap=$(symbol)) $^ $(LIBRARIES) -o $@

build/geome-fixture: build/src/main.o build/src/cli.o build/src/location.o build/src/output.o build/tests/fixture_http.o
	$(CC) $(LINK_FLAGS) $^ $(LIBRARIES) -o $@

build/validate_json: build/tests/validate_json.o
	$(CC) $(LINK_FLAGS) $^ $(LIBRARIES) -o $@

build/%.o: %.c build/config
	mkdir -p $(@D)
	$(CC) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

build/config: FORCE
	@mkdir -p build
	@printf '%s\n' '$(CC) $(COMPILE_FLAGS) $(LINK_FLAGS) $(LIBRARIES)' > build/config.new
	@cmp -s build/config.new $@ && rm build/config.new || mv build/config.new $@

sanitize:
	@mkdir -p build
	@$(CC) -fsanitize=address,undefined tests/compiler_probe.c -o build/sanitize-probe || { printf '%s\n' 'Compiler does not support ASan/UBSan.' >&2; exit 1; }
	$(MAKE) check HARDEN=0 CFLAGS="$(CFLAGS) -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined" LDFLAGS="$(LDFLAGS) -fsanitize=address,undefined"

install: all
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(MANDIR)/man1"
	install -m 0755 "$(PROGRAM)" "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	install -m 0644 man/geome.1 "$(DESTDIR)$(MANDIR)/man1/$(PROGRAM).1"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(PROGRAM)" "$(DESTDIR)$(MANDIR)/man1/$(PROGRAM).1"

DIST_FILES = LICENSE README.md CHANGELOG.md CONTRIBUTING.md Makefile .gitignore include src tests docs man debian .github
SOURCE_DATE_EPOCH ?= $(shell date +%s)
dist: clean
	tar --sort=name --mtime=@$(SOURCE_DATE_EPOCH) --owner=0 --group=0 --numeric-owner --exclude='debian/.debhelper' --exclude='debian/tmp' --exclude='debian/geome' --exclude='debian/geome-dbgsym' --exclude='debian/files' --exclude='debian/debhelper-build-stamp' --exclude='debian/*.substvars' --exclude='debian/*.debhelper*' --transform='s,^,geome-$(VERSION)/,' -czf geome-$(VERSION).tar.gz $(DIST_FILES)

clean:
	rm -rf build $(PROGRAM)

-include $(wildcard build/src/*.d build/tests/*.d)
